import pandas as pd
from datetime import datetime
from datetime import timedelta
import matplotlib.pyplot as plt

# This script creates and saves separate plots for the t-route 
# flows for each flowpath from the single resulting 
# flowveldepth.csv. This is configured to match the time 
# series from a ngen simulation's nexus outputs. Run with 
# "python plot_ngen_routing_flows.py", and no additional
# arguments are needed though the following three variables
# need to be set.

# Path to flowveldepth CSV.
# The current flowveldepth CSVs currently have timesteps 
# as integers and not the date-time. Therefore, any single
# nexus output file needs to be read in also by this script
# only for the date-times.
flowveldepth_df = pd.read_csv("flowveldepth_Ngen.csv", index_col=0)

# Path to any single nexus output file, which is needed
# only for the date-times.
nexus_output_df = pd.read_csv("nex-3030_output.csv", index_col=0)

# Set number of plots to be created. If plotting every flowpath,
# set to -1.
number_plots_to_create = -1

# Set the number of routing timesteps per hour.
# The current default routing timestep is 5 minutes,
# which is 300 seconds (dt = 300). Therefore, for a 
# 5 minute routing timestep, there will be 12 routing 
# timesteps per hour. The flowveldepth CSV contains 
# outputs for every timestep. For plotting purposes, 
# this reduces the outputs to every hour on the hour.
routing_timesteps_per_hour = 12

vel_and_depth_col_drop_list = []

# Drop velocity and depth columns
for col_index in range(len(flowveldepth_df.columns)):
  if (col_index % 3 == 0):
    pass
  else:
    vel_and_depth_col_drop_list.append(col_index)

flowveldepth_df.drop(flowveldepth_df.iloc[:, vel_and_depth_col_drop_list], inplace = True, axis = 1)

non_hourly_timestep_col_drop_list = []

# Drop non-hourly timestep columns 
for col_index in range(len(flowveldepth_df.columns)):
  if (col_index % routing_timesteps_per_hour == 0):
    pass
  else:
    non_hourly_timestep_col_drop_list.append(col_index)

flowveldepth_df.drop(flowveldepth_df.iloc[:, non_hourly_timestep_col_drop_list], inplace = True, axis = 1)

flow_timestep_list = flowveldepth_df.columns.tolist()

nexus_date_times_list = nexus_output_df.iloc[:, 0].tolist()

last_nexus_date_time = datetime.strptime(nexus_date_times_list[-1], " %Y-%m-%d %H:%M:%S")

date_time_delta_1_hour = timedelta(hours=1)

final_output_time = last_nexus_date_time + date_time_delta_1_hour

# Add addtional hour to account for last hour of routing
final_output_time_str = datetime.strftime(final_output_time, " %Y-%m-%d %H:%M:%S")

nexus_date_times_list.append(final_output_time_str)

# Create dictionary to change column names from timesteps to date-times
flow_timestep_to_time_dict = dict(zip(flow_timestep_list, nexus_date_times_list))

# Replace column names
flow_df = flowveldepth_df.rename(columns=flow_timestep_to_time_dict)

plt.rcParams['figure.figsize'] = [21, 12]

plot_index = 0

#Cycle through rows of dataframe and plot set of flows and and save to file
for index, row in flow_df.iterrows():

  if number_plots_to_create < 0 or plot_index < number_plots_to_create:

    plot_index += 1

    plt.xlabel("Date-Time")

    plt.ylabel("Flow (cms)")

    row.plot()

    # Rotate date-times on x axis to allow better visualization
    plt.xticks(rotation = 45)

    plt.title("Flowpath " + str(index))

    plt.savefig("Flowpath_" + str(index) + ".png")

    plt.clf()

