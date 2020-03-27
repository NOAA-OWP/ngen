# https://github.com/usgs-r/nhdplusTools
library(nhdplusTools)
# https://github.com/tidyverse/dplyr
library(dplyr)
# https://github.com/r-spatial/sf
library(sf)

library(hygeo)

sample_data <- system.file("gpkg/nhdplus_subset.gpkg", package = "hygeo")

png("demo.png")
plot_nhdplus(outlets = list(8895396),
             gpkg = sample_data, nhdplus_data = sample_data,
             overwrite = FALSE,
             plot_config = list(basin = list(border = NA),
                                outlets = list(default = list(col = NA))))

#st_layers("nhdplus_subset.gpkg")

fline <- read_sf(sample_data, "NHDFlowline_Network") %>%
  align_nhdplus_names() %>%
  filter(COMID %in% get_UT(., 8895396))

catchment <- read_sf(sample_data, "CatchmentSP") %>%
  align_nhdplus_names() %>%
  filter(FEATUREID %in% fline$COMID)

nexus <- get_nexus(fline)

plot(st_transform(st_geometry(catchment), 3857), add = TRUE)
plot(st_transform(st_geometry(nexus), 3857), add = TRUE)

dev.off()

unlink("rosm.cache", recursive = TRUE)

catchment_edge_list <- get_catchment_edges(fline)

waterbody_edge_list <- get_waterbody_edge_list(catchment_edge_list)

catchment_data <- get_catchment_data(catchment)

waterbody_data <- get_waterbody_data(fline)

nexus_data <- get_nexus_data(nexus)

write.csv(catchment_edge_list, "catchment_edge_list.csv", row.names = FALSE)

jsonlite::write_json(catchment_edge_list, "catchment_edge_list.json", pretty = TRUE)

write.csv(waterbody_edge_list, "waterbody_edge_list.csv", row.names = FALSE)

jsonlite::write_json(waterbody_edge_list, "waterbody_edge_list.json", pretty = TRUE)

write_sf(catchment_data, "catchment_data.geojson")

write_sf(waterbody_data, "waterbody_data.geojson")

write_sf(nexus_data, "nexus_data.geojson")

##### Code below runs hyRefactor of a larger region that
##### is a superset of the subset above.
# https://github.com/dblodgett-usgs/hyRefactor
# library(hyRefactor)
#
# source(system.file("extdata/new_hope_data.R", package = "hyRefactor"))
#
# refactor_nhdplus(nhdplus_flines = new_hope_flowline,
#                  split_flines_meters = 10000,
#                  collapse_flines_meters = 1200,
#                  collapse_flines_main_meters = 1200,
#                  split_flines_cores = 1,
#                  out_collapsed = "new_hope_refactor.gpkg",
#                  out_reconciled = "new_hope_reconcile.gpkg",
#                  three_pass = TRUE,
#                  purge_non_dendritic = FALSE,
#                  warn = FALSE)
#
# fline_ref <- sf::read_sf("new_hope_refactor.gpkg") %>%
#   sf::st_transform(proj)
# fline_rec <- sf::read_sf("new_hope_reconcile.gpkg") %>%
#   sf::st_transform(proj)
#
# cat_rec <- reconcile_catchment_divides(new_hope_catchment,
#                                        fline_ref, fline_rec,
#                                 new_hope_fdr, new_hope_fac)
#
# sf::write_sf(cat_rec, "new_hope_cat_rec.gpkg")


