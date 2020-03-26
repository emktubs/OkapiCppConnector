#include "../OkapiConnector.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>
#include <iostream>
#include <cstring>
#include <libnova/julian_day.h>

using std::cout;
using std::endl;
using std::string;
using std::vector;

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;

double altitude;
double longitude;
double latitude;
string start;
string endrt;

/**
 * This is a little helper function. it retrieves the result from the backend. Polls for the result until it is ready.
 */
OkapiConnector::CompleteResult retrieveResult(OkapiConnector connector, string baseUrl, string endpoint, string requestId)
{
  // First call to the backend
  OkapiConnector::CompleteResult result = connector.getResult(baseUrl, endpoint, requestId);
  if (result.error.code != 200 && result.error.code != 202)
  {
    cout << "Retrieving response failed with status: " << result.error.status << endl;
    cout << result.error.message << endl;
    return result;
  }
  int i = 0;
  // Poll the backend until the result is ready and can be retrieved
  while (result.error.code == 202) {
    result = connector.getResult(baseUrl, endpoint, requestId);
    cout << "The request was successful. Your result is not ready yet.  Waiting: " << i << " s." << endl;
    i++;
    sleep(1);
  }
  // Final call to the backend
  return connector.getResult(baseUrl, endpoint, requestId);
}

void readFilesFromDir(vector< string >& filenames, string dir_str)
{
    filenames.clear();
    char path[564];
    DIR *dir;
    struct dirent *ent;
    dir = opendir (dir_str.c_str());
    int passes_files = 0;
    if (dir != NULL) 
    {
        /* print all the files and directories within directory */
        struct stat buffer;
        while ((ent = readdir (dir)) != NULL) {
            //       printf ("%s\n", ent->d_name);
            string temp(ent->d_name);
            snprintf(path, 564, "%s/%s", dir_str.c_str(), ent->d_name);
            if( stat(path, &buffer) == 0 && S_ISREG( buffer.st_mode ) && temp.substr(temp.find_last_of(".")) == ".tle" )  
            {
                cout << string(path) << endl;
                filenames.push_back(string(path));
                passes_files++;
            }
        }
        closedir (dir);
    } 
    else {
        /* could not open directory */
        perror ("");
        return;
    }
}

double datetime2mjd(string fitstime)
{
  int year, month, day, hour, minute;
  double seconds;
  std::replace(fitstime.begin(), fitstime.end(), ':', ' ');
  std::replace(fitstime.begin(), fitstime.end(), 'T', ' ');
  std::replace(fitstime.begin(), fitstime.end(), '-', ' ');
  std::stringstream strstr(fitstime);
  strstr >> year >> month >> day >> hour >> minute >> seconds;
  
  struct ln_date date;
  date.years = year;
  date.months = month;
  date.days = day;
  date.hours = hour;
  date.minutes = minute;
  date.seconds = seconds;
  return ln_get_julian_day(&date)-2400000.5;
}



void readTLE(vector< string >& tle, string tle_file) 
{
  std::ifstream ifile(tle_file, std::ifstream::in);
  string line1, line2;
  for(; getline( ifile, line1 ); )
  {
      if( line1.at(0)=='1' ) {
        getline( ifile, line2 );
        tle.push_back(line1 + '\n' + line2);
      }
  }
}

OkapiConnector::CompleteResult tle2okapiResult(OkapiConnector connector, string tle)
{
  // Correct URL and port for the v2020.01 release
  string baseUrl = "http://okapi.ddns.net:34568";
  
  double outputTimeStep = 1;
  // preparation for pass prediction using SGP4
  web::json::value simpleGroundLocation;
  simpleGroundLocation[U("altitude")] = web::json::value::number(altitude);
  simpleGroundLocation[U("longitude")] = web::json::value::number(longitude);
  simpleGroundLocation[U("latitude")] = web::json::value::number(latitude);
  web::json::value timeWindow;
  timeWindow[U("start")] = web::json::value::string(start);
  timeWindow[U("end")] = web::json::value::string(endrt);
  web::json::value predictPassesSettingsSimple;
  predictPassesSettingsSimple[U("predict_passes_settings")] = web::json::value::number(outputTimeStep);
  web::json::value passPredictionRequestBody;
  passPredictionRequestBody[U("simple_ground_location")] = simpleGroundLocation;
  passPredictionRequestBody[U("predict_passes_settings")] = predictPassesSettingsSimple;
  passPredictionRequestBody[U("time_window")] = timeWindow;
  passPredictionRequestBody[U("tle")] = web::json::value::string(tle);

  
  // Send request for SGP4 pass prediction
  OkapiConnector::CompleteResult responseSGP4 = connector.sendRequest(baseUrl, "/predict-passes/sgp4/requests", passPredictionRequestBody);
  if (responseSGP4.error.code != 200 && responseSGP4.error.code != 202)
  {
    cout << "SGP4 request failed with status: " << responseSGP4.error.status << endl;
    cout << responseSGP4.error.message << endl;
  }
  string requestIdPassPredictionSgp4 = connector.requestId;
  cout << "SGP4 request ID: " << requestIdPassPredictionSgp4 << endl;
  
  // Get results for SGP4 and print them in the terminal
  OkapiConnector::CompleteResult sgp4SimpleResult = retrieveResult(connector, baseUrl, "/predict-passes/sgp4/simple/results/", requestIdPassPredictionSgp4);
  if (sgp4SimpleResult.error.code != 200 && sgp4SimpleResult.error.code != 202)
  {
    cout << "Response failed with status: " << sgp4SimpleResult.error.status << endl;
    cout << sgp4SimpleResult.error.message << endl;
  }
//   else
//   {
//     return sgp4SimpleResult;
//     auto DataArray = sgp4SimpleResult.body.as_array();
// 
//     for (auto Iter = DataArray.begin(); Iter != DataArray.end(); ++Iter)
//     { 
//         cout << "New Pass" << endl;
//         auto& data = *Iter;
//         auto dataObj = data.as_object();
// 
//         for (auto iterInner = dataObj.cbegin(); iterInner != dataObj.cend(); ++iterInner)
//         {
//             auto &propertyName = iterInner->first;
// //             auto &propertyValue = iterInner->second;
// 
//             cout << "Property: " << propertyName << endl;
//         }
//     }
//     cout << sgp4SimpleResult.body.serialize() << endl;
//   }

//   OkapiConnector::CompleteResult sgp4SummaryResult = retrieveResult(connector, baseUrl, "/predict-passes/sgp4/summary/results/", requestIdPassPredictionSgp4);
//   if (sgp4SummaryResult.error.code != 200 && sgp4SummaryResult.error.code != 202)
//   {
//     cout << "Response failed with status: " << sgp4SummaryResult.error.status << endl;
//     cout << sgp4SummaryResult.error.message << endl;
//   }
//   else
//   {
//     cout << sgp4SummaryResult.body.serialize() << endl;
//   }
  
  return sgp4SimpleResult;
}

double fix_azimuth(double azimuth)
{
  if( azimuth<0 )
    return azimuth+360;
  else
    return azimuth;
}

void okapiResult2PassFile(OkapiConnector::CompleteResult okares, string filename, string id)
{
  auto DataArray = okares.body.as_array();
  std::ofstream ofile(filename, std::ofstream::out | std::ofstream::app);
  for (auto Iter = DataArray.begin(); Iter != DataArray.end(); ++Iter)
  { 
    cout << "[New Pass]" << endl;
    auto& data = *Iter;
    
    auto azimuths_ptr = data.at(U("azimuths")).as_array();
    auto elevations_ptr = data.at(U("elevations")).as_array();
    auto in_sun_lights_ptr = data.at(U("in_sun_lights")).as_array();
    auto ranges_ptr = data.at(U("ranges")).as_array();
    auto time_stamps_ptr = data.at(U("time_stamps")).as_array();

    auto azimuths_iter = azimuths_ptr.cbegin();
    auto elevations_iter = elevations_ptr.cbegin();
    auto in_sun_lights_iter = in_sun_lights_ptr.cbegin();
    auto ranges_iter = ranges_ptr.cbegin();
    auto time_stamps_iter = time_stamps_ptr.cbegin();
    
    while ( azimuths_iter != azimuths_ptr.cend() )
    {
      if( in_sun_lights_iter->as_integer()==1 ) {
        ofile << id << '\t'
              << std::fixed << std::setprecision(8)
              << datetime2mjd(time_stamps_iter->as_string()) << '\t'
              << std::fixed << std::setprecision(3)
              << fix_azimuth(azimuths_iter->as_double()) << '\t'
              << elevations_iter->as_double() << '\t'
              << ranges_iter->as_double() << '\t'
              << "0.0\t0.0\t0.0" 
              << endl;
      }
      
      azimuths_iter++;
      elevations_iter++;
      in_sun_lights_iter++;
      ranges_iter++;
      time_stamps_iter++;
    }
  }
  ofile.close();
}

int main(int argc, char* argv[])
{
  // initializing communication
  OkapiConnector connector;
  
  //some storage variables
  vector< string > tle_vec;
  vector< string > files_vec;
  string output_file("../output.dat");
 
  // User input for authentication
  std::ifstream ifile("../okapi_acc", std::ifstream::in);
  string username, password;
  getline( ifile, username );
  getline( ifile, password );
  ifile.close();
  // Here you add your username (should be an e-mail address):
//   string username = "";
  // Here you add your password:
//   string password = "";
  // Correct URL and port for the v2020.01 release
  string baseUrl = "http://okapi.ddns.net:34568";
  
	// Authentication with Auth0 to retrieve the access token
  cout << "[Authentication] - started" << endl;
	OkapiConnector::CompleteResult initResult
      = connector.init(methods::POST,username,password);
  
  if (initResult.error.code == 200 || initResult.error.code == 202)
  {
    cout << "[Authentication] - completed" << endl;
  }
  else
  {
    cout << "[Authentication] - failed with status: " << initResult.error.status << endl;
    cout << initResult.error.message << endl;
    return -1;
  }

  // PASS PREDICTION
  cout << "[Predict passes] - started" << endl;
// user input PASS PREDICTION
  std::ifstream obsinf("../obsinf", std::ifstream::in);
  obsinf >> altitude;
  obsinf >> longitude;
  obsinf >> latitude;
  obsinf >> start;
  obsinf >> endrt;
  obsinf.close();
  
  readFilesFromDir(files_vec, string("../tle"));
  vector< string > tmp_tle;
  if( !files_vec.empty() ) {
    for( vector<string>::iterator files_it = files_vec.begin() ; files_it != files_vec.end(); ++files_it )
    {
      readTLE(tmp_tle, *files_it);
      if( !tmp_tle.empty() ) {
        for( vector<string>::iterator tle_it = tmp_tle.begin() ; tle_it != tmp_tle.end(); ++tle_it )
        {
          tle_vec.push_back(*tle_it);
        }
      }
    }
  }
  else {
    cout << "No TLE files found" << endl;
    return -1;
  }
  
  if( !tle_vec.empty() ) {
    for(vector<string>::iterator tle_it = tmp_tle.begin() ; tle_it != tmp_tle.end(); ++tle_it )
    {
      cout << "[Processing TLE]" << endl << *tle_it << endl;
      string id = tle_it->substr(2,5);
      cout << "[Extracted ID] - " << id << endl;
      
      OkapiConnector::CompleteResult result = tle2okapiResult(connector, *tle_it);
      
      okapiResult2PassFile(result, output_file, id);
      
    }
  }
  
  cout << "[Predict passes] - completed" << endl;



  return 0;
}
