import geopandas as gpd
import pandas as pd
import json
import argparse
from shapely.geometry import Point

def main():
    #setup the argument parser
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", dest="infile", type=str, required=True, help="A gpkg file containing divides and nexus layers")
    parser.add_argument("-C", dest="catchments_prime", type=str, required=True, help="Hydrofabric catchments destination")
    parser.add_argument("-N", dest="nexuses_prime", type=str, required=True, help="Hydrofabric nexus desination")
    args = parser.parse_args()

    infile = args.infile
    catchments_prime = args.catchments_prime
    nexuses_prime = args.nexuses_prime

    print("Reading catchment data...")
    df_cat = gpd.read_file(str(infile), layer="divides")

    print("Reading nexus data...")
    df_nex = gpd.read_file(str(infile), layer="nexus")

    df_cat.set_index('id', inplace=True)
    df_nex.set_index('id', inplace=True)

    print("Reprojecting catchments...")
    df_cat = df_cat.to_crs(epsg=4326)

    print("Reprojecting nexuses...")
    df_nex = df_nex.to_crs(epsg=4326)


    print("Writing catchment data...")
    df_cat.to_file(catchments_prime, driver='GeoJSON')
    print("Writing nexus data...")
    df_nex.to_file(nexuses_prime, driver='GeoJSON')
    print("Complete!")

if __name__ == "__main__":
    main()
