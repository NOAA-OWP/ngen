# Testing BMI Python modules.
 - AORC_model.py: This file is the "model" it takes inputs and gives an output
 - AORC_bmi_model.py: This is the Basic Model Interface that talks with the model. This is what we are testing.
 - AORC_run_bmi_model.py: This is a file that mimics the framework, in the sense that it initializes the model with the BMI function. Then it runs the model with the BMI Update function, etc.
 - AORC_run_bmi_unit_test.py: This is a file that runs each BMI unit test to make sure that the BMI is complete and functioning as expected.
 - config.yml: This is a configuration file that the BMI reads to set inital_time (initial value of current_model_time) and time_step_seconds (time_step_size).
 - environment.yml: Environment file with the required Python libraries needed to run the model with BMI. Create the environment with this command: `conda env create -f environment.yml`, then activate it with `conda activate bmi_test`

# Running the Module in ngen Framework
This module has been set up to run in the ngen framework. To run it, in the same environment set up for the testing, execute `python run_ngen_aorc_bmi.py`. The python script contains command to run ngen through time steps. Similar to running ngen in Linux environment, several input files are needed. We have provided some example files for a test run, in addition to files used in "Testing BMI Python modules" step. These include forcing files in data/forcing/ and initialization files in data/bmi/ subdirectories. Specific catchment ids are in config.yml and hydrofabric file names used can be found in "run_ngen_aorc_bmi.py".

# About
This is an implementation of a Python-based model that fulfills the Python language BMI interface and can be used in the Framework. It not only serves as a control for testing purposes, but can also run ngen models directly within the ngen framework.

# Implementation Details

## Test the complete BMI functionality
`python AORC_run_bmi_unit_test.py`

## Run the model
`python AORC_run_bmi_model.py`

## Sample output
time,ids,APCP_surface,DLWRF_surface,DSWRF_surface,PRES_surface,SPFH_2maboveground,TMP_2maboveground,UGRD_10maboveground,VGRD_10maboveground  
Number of catchments =  3  
3600,cat-304922,0.19598033720225594,354.64431680804955,0.0,95638.33984375,0.010811518586741342,288.0360882764471,0.3059325740625347,4.125184692817697  
3600,cat-298118,0.0,362.28291555468604,0.0,94354.31640625,0.008793832175310046,284.8541058071569,-0.15734371181344553,4.9149899261443295  
3600,cat-313395,0.0,372.4068414867943,0.0,97642.36328125,0.010804456819341612,288.98330508743675,-2.2181629511440235,4.160564484604663  
7200,cat-304922,0.19770582016864147,354.07603066676893,0.0,95657.587890625,0.01045194370568825,287.48503846354834,0.14260510418720695,4.026551497378222  
7200,cat-298118,1.16875430895022,360.88564990886516,0.0,94328.828125,0.008599194881643013,284.4297161523955,-0.3954913675113225,4.740259622637325  
7200,cat-313395,0.02261771891441522,372.069903005211,0.0,97673.4375,0.010601139563735651,288.7557171934204,-2.115987999975502,4.66720053951741  
10800,cat-304922,0.0,353.8111380846967,0.0,95677.578125,0.010130283099800408,287.0018597454109,-0.016926667346457647,3.941462003312438  
10800,cat-298118,0.0,360.76062549293965,0.0,94315.205078125,0.008499300932750531,284.2472942746772,-0.6302241419289132,4.569853278545338  
10800,cat-313395,0.0,372.2814020318001,0.0,97705.849609375,0.010454051707343981,288.64841738713403,-2.0056255639437666,5.178488999284639  
14400,cat-304922,0.0,361.6074760914962,0.0,95697.265625,0.009824299373399348,286.52222106638874,-0.17886626986959975,3.844092617095896  
14400,cat-298118,0.0,354.927812906024,0.0,94289.90234375,0.008410385682311172,284.0672405610567,-0.8741713654126428,4.444043034971401  
14400,cat-313395,0.6864187819722112,376.55552318923947,0.0,97734.326171875,0.010315466810942586,288.5502484403587,-1.9056255624536504,5.696982277884814  
18000,cat-304922,0.7075690374906216,361.13733448292805,0.0,95663.80859375,0.009683531707277704,286.48303649549234,0.042605102697090835,4.120328964595927  
18000,cat-298118,0.20000000298023224,354.6764701288412,0.0,94254.6875,0.008408973481419113,284.0672405610567,-0.5617199027794584,4.844043422401597  
18000,cat-313395,0.19975340664019292,376.79871166942576,0.0,97701.826171875,0.010205715684564742,288.49382754108046,-1.1233993697672986,5.663176811682831  
21600,cat-304922,2.3378218045506145,360.67288135100716,0.0,95634.2578125,0.009522573611978613,286.45388610443297,0.27379851749239137,4.349398487052298  
21600,cat-298118,0.019107242213297515,354.37070840553497,0.0,94221.396484375,0.0083930051590686,284.06799739700546,-0.25144698994527737,5.24404304689233  
21600,cat-313395,0.0,377.05476636074127,0.0,97671.15234375,0.010100184376195531,288.42436953223296,-0.31598804467898667,5.6057579875850365

## Run the ngen model
`python run_ngen_aorc_bmi.py`
