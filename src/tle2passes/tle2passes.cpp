#include "../OkapiConnector.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>
#include <iostream>
#include <cstring>

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
  
  return sgp4SimpleResult;
}

void okapiResult2PassFile(OkapiConnector::CompleteResult okares, string filename, string id)
{
  
}

int main(int argc, char* argv[])
{
  // initializing communication
  OkapiConnector connector;
 
  // User input for authentication
  // Here you add your username (should be an e-mail address):
  string username = "eriklein@tu-braunschweig.de";
  // Here you add your password:
  string password = "ok_+-Goodluck";
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
  cout << "Please enter altitude:" << endl;
  cin >> altitude;
  cout << "Please enter longitude:" << endl;
  cin >> longitude;
  cout << "Please enter latitude:" << endl;
  cin >> latitude;
  cout << "Please enter start time of prediction :" << endl;
  cout << "Example format: 2018-08-06T18:19:44.256628Z";
  cin >> start;
  cout << "Please enter end time of prediction (same format as start time):" << endl;
  cin >> endrt;
  //  string start = "2018-08-07T17:30:00.000Z";
  //  string end =   "2018-08-07T17:31:00.000Z";
//   string tlePassPrediction = "1 25544U 98067A   18218.76369510  .00001449  00000-0  29472-4 0  9993\n2 25544  51.6423 126.6422 0005481  33.3092  62.9075 15.53806849126382";


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
  
  cout << "[Predict passes] - completed" << endl;



  return 0;
}
