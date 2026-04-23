import json
import argparse
import pandas as pd
import numpy as np
import geopandas as gpd
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from itertools import cycle
​
import seaborn as sns
from pathlib import Path
​
categories = ['CFE', 'LSTM', 'NoahOWP+CFE']
#categories = ['CFE', 'lstm', 'bmi_fortran_noahmp+bmi_c_cfe']
​
def get_formulation_meta(formulation: dict, catchment=-1):
    """_summary_
​
    Args:
        formulation (dict): _description_
    """
    mapping = {'name':[], 'type':[]}
    name = formulation['name']
    if name == 'bmi_multi':
        #recurse through the modules
        
        for module in formulation['params']['modules']:
            nested = get_formulation_meta(module)
            mapping['name'].extend(nested['name'])
            mapping['type'].extend(nested['type'])
    else:
        type = formulation['params'].get('model_type_name', None)
        mapping['name'].append(name)
        mapping['type'].append(type)
    return mapping
​
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-p','--partitions', required=True, type=argparse.FileType('r'))
    parser.add_argument('-c','--catchments', required=True, type=Path)
    parser.add_argument('-o','--out', default='partitions.png')
    parser.add_argument('-m', '--colormap', default='Spectral')
    parser.add_argument('-t', '--title', help="Title to add to generated plot", default=None)
    parser.add_argument('-n', '--numcolors', help="color number to pick from colormap", type=int, default=0)
    parser.add_argument('-s', '--simple', help='Turn off all titles and labels', action='store_true')
    args = parser.parse_args()
​
​
    #with open(args.partitions) as f:
    data = json.load(args.partitions)
    df_c = pd.json_normalize(data, record_path =['partitions','cat-ids'], meta=[['partitions','id']])
    df_c = df_c.rename(columns={0: "id", "partitions.id": "partition_id"})
​
    gdf_c = gpd.read_file(args.catchments)
    gdf_c = gdf_c.merge(df_c, on='id')
    partitions = gdf_c['partition_id'].unique()
    n_colors = len(partitions)#gdf['model_type'].unique().size
    cmap = sns.color_palette(args.colormap, as_cmap=True)
    palette = dict(zip(sorted(partitions),
               [cmap(x*50) for x in range(n_colors)]))
    #print(palette)
    colors = [color for category, color in palette.items()
                           if category in gdf_c['partition_id'].values ]
    #print(colors)
    cmap = ListedColormap(colors)
    #print(cmap)
    #plt.set_cmap(cmap)
    fig = plt.figure()
    ax = fig.gca()
    #c = plt.cycler('color', cmap(np.linspace(.1,1, n_colors)) )
    # supply cycler to the rcParam
    #plt.rcParams["axes.prop_cycle"] = c
    for _ in range(args.numcolors):
        ax._get_lines.get_next_color()
    e_color = 'black'
    e_color = 'none'
    l_pos = 'lower left'
    l_pos = 'upper left'
    #quick hack for huc01 label fixup
​
    ax = gdf_c.plot(cmap=cmap, column='partition_id', legend=True, legend_kwds={'fontsize':'small', 'loc': l_pos}, categorical=True, edgecolor=e_color)
    #ax = gdf.plot(cmap=cmap, column='model_type', legend=False, legend_kwds={'fontsize':'small', 'loc': l_pos}, categorical=True, edgecolor=e_color)
​
    #flowpaths = flowpaths.to_crs(gdf.crs)
    #flowpaths.plot(flowpaths, ax=ax, color='blue')
​
    if( args.simple ):
        ax.set_axis_off()
    else:
        plt.suptitle(args.title)
        plt.title("NGen Parallel Partitions")
    plt.savefig(args.out, dpi=1200, bbox_inches='tight', 
                transparent=True,
                pad_inches=0)
