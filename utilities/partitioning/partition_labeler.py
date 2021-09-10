import json
import argparse
import pandas as pd
import geopandas as gpd

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('-p','--partitions', type=argparse.FileType('r'))
    parser.add_argument('-c','--catchments', type=argparse.FileType('r'))
    parser.add_argument('-n','--nexi','--nexuses', type=argparse.FileType('r'))
    parser.add_argument('-o','--out', default='hydrofabric_partitioned.geojson')
    args = parser.parse_args()

    if(args.catchments is None and args.nexi is None):
        parser.error("Either --catchments or --nexi is required.")

    #with open(args.partitions) as f:
    data = json.load(args.partitions)

    gdf_c = None
    gdf_n = None
    if args.catchments:
        df_c = pd.json_normalize(data, record_path =['partitions','cat-ids'], meta=[['partitions','id']])
        df_c = df_c.rename(columns={0: "id", "partitions.id": "partition_id"})

        gdf_c = gpd.read_file(args.catchments)
        gdf_c = gdf_c.merge(df_c, on='id')
    
    if args.nexi:
        df_n = pd.json_normalize(data, record_path =['partitions','nex-ids'], meta=[['partitions','id']])
        df_n = df_n.rename(columns={0: "id", "partitions.id": "partition_id"})

        gdf_n = gpd.read_file(args.nexi)
        gdf_n = gdf_n.merge(df_n, on='id')

    gdf_f = gdf_c
    if gdf_n is not None:
        if gdf_f is not None:
            gdf_f = gdf_f.append(gdf_n)
        else:
            gdf_f = gdf_n
            
    gdf_f.to_file(args.out, driver='GeoJSON')


    