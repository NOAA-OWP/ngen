#!/usr/bin/env python
import pandas as pd
import numpy as np
import geopandas as gpd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, PillowWriter, FFMpegWriter
from matplotlib.colors import ListedColormap
from itertools import cycle
import shapely as shp
import seaborn as sns
from pathlib import Path
import xarray as xr
from functools import partial

import os
catchments = gpd.read_file("gauge_01073000.gpkg", layer="divides", include_fields=['divide_id'])
forcing = xr.open_dataset("data/Long_Range_Hydrofabric_Precip.nc", decode_cf=False)

forecast="Short Range" #TODO add product (CFS, GFS ect...)
forecast="Long Range" #TODO add product (CFS, GFS ect...)

forcing.time.attrs['units'] = forcing.time.attrs['reference_time']
forcing = xr.decode_cf(forcing)
forcing = forcing.set_coords("catchment_ids")

cmap = sns.color_palette('flare', as_cmap=True)

e_color = 'none'
vmin = forcing['RAINRATE'].min().item()
vmax = forcing['RAINRATE'].max().item()

def plot_frame(ax, i):
    
    t = forcing['RAINRATE'][i:i+1]
    #print(t)
    #each dataset is a slice of time
    ts = t.time.dt.strftime("%Y-%m-%d %H:%M:%S")[0].item()
    
    df = t.to_dataframe().droplevel(0)
    df['catchment_ids'] = df['catchment_ids'].apply(lambda x: "cat-"+str(x))
    #merge on catchment ids
    rdf = catchments.merge(df, left_on="divide_id", right_on="catchment_ids")
    #rdf.plot(ax=ax, column="RAINRATE", cmap=cmap, legend=False, categorical=False, edgecolor=e_color)
    ax.set_title(forecast+"\n"+ts+"\nPrecipitation", fontsize=20)
    #FIXME don't plot new, update old!!!
    ax.collections[0].set_array(rdf['RAINRATE'].values)
    #ax.redraw_in_frame()
    print("Plotting time ", i)
    return ax

fig = plt.figure(figsize=(19.20, 10.80))
ax = fig.gca()
#plt.suptitle("Short Range")
cax = fig.add_axes([0.9, 0.1, 0.03, 0.8])

sm = plt.cm.ScalarMappable(cmap=cmap, norm=plt.Normalize(vmin=vmin, vmax=vmax))
# fake up the array of the scalar mappable. Urgh...
sm._A = []
cbar = fig.colorbar(sm, cax=cax)
cbar.set_label(forcing['RAINRATE'].attrs['units'])
#ax = fig.gca()
ax.set_axis_off()
ax.title.set_size(20)

ax = catchments.plot(ax=ax, legend=False, edgecolor=e_color, facecolor='white')
ax.collections[0].set_cmap(cmap)
ax.collections[0].set_clim(vmin, vmax)

print("NUM FRAMES")
print(forcing.coords['time'].size)
#plot_frame(ax, 224)
ani = FuncAnimation(fig, partial(plot_frame, ax), frames=forcing.coords['time'].size, interval=100)
#plt.show()
#ani.save("forcings_"+forecast.replace(" ","_")+".gif", dpi=1200, writer=PillowWriter(fps=20))
plt.tight_layout()
plt.subplots_adjust(top=0.85) 
ani.save("forcings_"+forecast.replace(" ","_")+".mp4", writer=FFMpegWriter(fps=60) ) 
plt.close() 
#plt.savefig("test.png", dpi=1200, bbox_inches='tight', 
#            transparent=True,
#            pad_inches=0)