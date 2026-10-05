#!/usr/bin/env python
"""
@author Nels J. Frazier
@date 01-02-2024
@version 0.0.0

Utility to generate animations in gif format in both gridded form
and catchment aggregated form.  Use forcing_animation --help for
descriptions of options and required inputs.

"""
import geopandas as gpd
import matplotlib.pyplot as plt
#import PIL
#PIL.Image.MAX_IMAGE_PIXELS = 188745354+10000
from matplotlib.animation import FuncAnimation, PillowWriter, FFMpegWriter
import seaborn as sns
import xarray as xr
from functools import partial
import cartopy.crs as ccrs
import cartopy.feature as cfeature
#import pickle
import argparse
from pyogrio import read_dataframe

def plot_catchment_frame(ax, forecast, forcing, catchments, i):
    t = forcing[i]
    #each dataset is a slice of time
    ts = t.time.dt.strftime("%Y-%m-%d %H:%M:%S").item()
    ax.set_title(forecast+"\n"+ts+"\nPrecipitation", fontsize=20)
    #don't plot new, update old!!!
    ax.collections[0].set_array(t.values)
    print("Plotting time ", i)
    return ax

from matplotlib.collections import QuadMesh
def plot_grid_frame(ax, forecast, forcing, cmap, i):
    #Get time at int index i
    t = forcing['RAINRATE'][i:i+1]
    ts = t.time.dt.strftime("%Y-%m-%d %H:%M:%S").item() 
    ax.axes.set_title(forecast+"\n"+ts+"\nPrecipitation", fontsize=20)
    #update 
    ax.set_array(t[0].values)
    print("Plotting time ", i)
    return ax

cycles = ["Short", "Medium", "Long"]
sources = ['HRRR-RAP', 'CFS', 'GFS', 'Other']

parser = argparse.ArgumentParser()
#TODO add cariable arg
#TODO add cmap arg
parser.add_argument('-p', '--preview', help="Plot the first frame and view only", required=False, action="store_true")
parser.add_argument('-s', '--switch', help="switch the orientation of the colorbar from horizontal to vertical", required=False, action="store_true")
parser.add_argument('-d', '--dpi', required=False, default=100, help="Resolution to save gif", type=int)
parser.add_argument("source", choices=sources)
parser.add_argument("cycle", choices=cycles)
commands = parser.add_subparsers(dest="command")

h_commands = commands.add_parser("hydrofabric")
h_commands.add_argument('hydrofabric', metavar="hydrofabric geopackage", type=argparse.FileType('r'))
h_commands.add_argument('data', metavar="netcdf precip", type=argparse.FileType('r'))

g_commands = commands.add_parser("grid")
g_commands.add_argument('data', metavar="gridded netcdf precip", type=argparse.FileType('r'))

args = parser.parse_args()

forecast = args.cycle+" Range "+args.source
crs = ''
epsg = ''
if(args.command == 'hydrofabric'):
    epsg = '5070'
    crs = ccrs.epsg(epsg)
    crs = ccrs.PlateCarree()
elif(args.command == 'grid'):
    epsg = '4326'
    # doesn't work: crs= ccrs.epsg('4326')
    crs = ccrs.PlateCarree() #NJF why this one???

forcing = xr.open_dataset(args.data.name, decode_cf=False)
forcing.time.attrs['units'] = forcing.time.attrs['reference_time']
forcing = xr.decode_cf(forcing)

###Setup general figure
#cmap = sns.color_palette('flare', as_cmap=True)
#Single hughe blue, really light on the 0 side...
#cmap = sns.color_palette('Blues', as_cmap=True)
#Little better constrast, still high desat on the low end
cmap = sns.color_palette("light:b", as_cmap=True)
#Reasonably good "blue ish" cube helix
#cmap = sns.color_palette("ch:start=.2,rot=-.2", as_cmap=True)

bkgrnd = sns.color_palette('flare', as_cmap=True)
#cmap.set_under(bkgrnd(50), alpha=0.05)
cmap.set_under("bisque", alpha=0.1)
e_color = 'none'
vmin = forcing['RAINRATE'].min().item()
vmin = 0.0009
vmax = forcing['RAINRATE'].max().item()

fig = plt.figure(figsize=(19.20, 10.80), dpi=100)
ax = plt.axes(projection=crs)
#plt.suptitle("Short Range")
if(args.switch):
    cax = fig.add_axes([0.9, 0.1, 0.03, 0.8]) #vertical
    orient = "vertical"
else:
    cax = fig.add_axes([0.05, 0.05, 0.9, 0.03]) #horizontal
    orient = "horizontal"
    norm = plt.Normalize(vmin=vmin, vmax=vmax)
sm = plt.cm.ScalarMappable(cmap=cmap, norm=norm)
# fake up the array of the scalar mappable. Urgh...
sm._A = []

cbar = fig.colorbar(sm, cax=cax, orientation=orient)
cbar.set_label(forcing['RAINRATE'].attrs['units'])
#ax = fig.gca()
ax.set_axis_off()
ax.title.set_size(20)

interval = 100
fps = 20
if(args.cycle == "Short"):
    interval = 5
    fps = 10

if(args.command == "hydrofabric"):
    fs = forcing["catchment_ids"].to_series()
    print( fs[ fs.duplicated() ].values)
    # WAY faster reading with pyogrio and use_arrow (though just pyogrio is a significant imporvement!)
    catchments = gpd.read_file(args.hydrofabric.name, layer="divides", include_fields=['divide_id'], engine='pyogrio', use_arrow=True)
    # union = gpd.GeoSeries( catchments.unary_union, crs=catchments.crs)
    union = gpd.read_file("domain.gpkg", engine='pyogrio')
    
    print("Projecting domain to epsg:4326")
    union = union.to_crs(epsg='4326')
    catchments = catchments.to_crs(epsg='4326')

    forcing = forcing.set_index({"catchment ids":"catchment_ids"}).to_array().squeeze().drop_vars('variable')
    #Get the catchment geopandas index, strip to int for xarray index
    cat_ids = catchments['divide_id'].apply(lambda x: int(x.split("-")[1]))
    #This is one way to re-org the data along the geopandas index
    #forcing = forcing.loc[:, cat_ids.values]
    #But this should be much faster!
    #FIXME issues with duplicate ids in forcing files...
    forcing = forcing.drop_duplicates("catchment ids")
    forcing = forcing.reindex({"catchment ids":cat_ids.values})
    # This was a good idea -- cache the axes, but won't work for general usage
    # and also just doesn't work to deserialize the GeoAxes from cartopy...
    # if( Path(".catchment_axes_caches").exists() ):
    #     with open('.catchment_axes_caches', 'rb') as cache:
    #         ax = pickle.load(cache)
    # else:
    #     ax = catchments.plot(ax=ax, legend=False, edgecolor=e_color, facecolor='white')
    #     with open('.catchment_axes_caches', 'wb') as cache:
    #         pickle.dump(ax, cache)

    # epsg 5070 projected bounds: -7013714.59 967179.63 -5299735.56 2630462.94
    # This cuts off florida and texas!!! For now, just reproject to wgs84 and plot
    # on the PlateCarree projection
    # might consider trying 
    print("Plotting base...")
    ax.add_geometries( union.geometry, facecolor="none", edgecolor="black", crs=crs)
    #ax.set_extent( catchments.total_bounds, crs=crs )
    box = catchments.total_bounds
    ax.set_xlim((box[0], box[2]))
    ax.set_ylim((box[1], box[3]))
    ax = catchments.plot(ax=ax, legend=False, edgecolor=e_color, facecolor='white', vmin=vmin, vmax=vmax, norm=norm)
    #FIXME try the union before projecting, then re-project the resulting geometry
    # Turns out, this fixes the topology error, but is slow, for now hacking in Mike's outline
    #ax.add_geometries( [catchments.unary_union], crs=crs, facecolor=None, edgecolor="black")
    ax.collections[0].set_cmap(cmap)
    ax.collections[0].set_clim(vmin, vmax)
    
    #ax.add_feature(cfeature.STATES.with_scale('50m'))
    if(args.preview):
        plot_catchment_frame(ax, forecast, forcing, cat_ids, 6)
    else:
        ani = FuncAnimation(fig, partial(plot_catchment_frame, ax, forecast, forcing, cat_ids), frames=forcing.coords['time'].size, interval=interval)
elif(args.command == "grid"):
    ax.add_feature(cfeature.STATES.with_scale('110m'))
    ax = forcing['RAINRATE'].isel(time=0).plot(ax=ax, cmap=cmap, add_colorbar=False, vmin=vmin, vmax=vmax, norm=norm)
    # print(ax)
    #plt.show()
    # os._exit(1)
    #odr = cartopy.feature.NaturalEarthFeature(category='cultural', 
    #name='admin_0_boundary_lines_land', scale='50m', facecolor='none', alpha=0.7)
    #ax.add_feature(odr)
    if(args.preview):
        plot_grid_frame( ax, forecast, forcing, cmap, 0)
    else:
        ani = FuncAnimation(fig, partial(plot_grid_frame, ax, forecast, forcing, cmap), frames=forcing.coords['time'].size, interval=interval)



# catchments = gpd.read_file("gauge_01073000.gpkg", layer="divides", include_fields=['divide_id'])
# forcing = xr.open_dataset("data/Long_Range_Hydrofabric_Precip.nc", decode_cf=False)

# forecast="Short Range" #TODO add product (CFS, GFS ect...)
# forecast="Long Range" #TODO add product (CFS, GFS ect...)


print("NUM FRAMES")
print(forcing.coords['time'].size)
#plot_frame(ax, 224)

#plt.show()

#ani.save("forcings_"+forecast.replace(" ","_")+".gif", dpi=1200, writer=PillowWriter(fps=20))
#plt.tight_layout()
#plt.subplots_adjust(top=0.85) 
if(args.preview):
    plt.show()
else:
    #ani.save("forcings_"+forecast.replace(" ","_")+".mp4", writer=FFMpegWriter(fps=fps) )
    ani.save(args.command+"_forcings_"+forecast.replace(" ","_")+".gif", writer="ffmpeg", dpi=args.dpi)
plt.close() 
#plt.savefig("test.png", dpi=1200, bbox_inches='tight', 
#            transparent=True,
#            pad_inches=0)