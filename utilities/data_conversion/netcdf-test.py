import netCDF4 as nc

def test1():
    print("Test what happends if you try to open the same netcdf file for update more than once in the same processes")

    filename = "/local/ngen/data/huc01/huc_01/forcing/netcdf/huc01.nc"
    try:
        ds1 = nc.Dataset(filename, 'r+', format='NETCDF4')
        ds2 = nc.Dataset(filename, 'r+', format='NETCDF4')
        ds1.close()
        ds2.close()
        print("Test passed")
    except OSError as e: 
        print(e)
        print("Test failed")



def main():
    test1()

if __name__ == "__main__":
    main()