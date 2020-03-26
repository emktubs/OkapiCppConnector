# OkapiCppConnector
Provides basic routines to init your OKAPI account as well as send requests and receive results.

### What you need to build
* a C++ compiler: e.g. gcc or clang
* CMake: we used Verison 3.2
* a text editor or IDE of your choice
* a terminal
* libraries: boost_system, boost_thread, boost_chrono, crypto, ssl and cpprest
* this was tested with Ubuntu 18.04

### The build process
To build the library, create a `build` directory in your main folder, change into it and run the two commands `cmake ../` and `make install`. This will generate a shared object (.so or .dylib) in the `lib` folder. You can now use the library. The runnable example `okapi-connector-test` is available in the `bin`directory. 

For more information on the API and more examples, visit www.okapiorbits.space/documentation/

### Use the TLE to Passes executable
Follow the above instructions to build the executable. Further add your observer information in the following order to file "obsinf":
- altitude
- longitude
- latitude
- start time
- end time

Have a look into the existing "obsinf" file for an example. The format of the time stamps have to be of the following:
- YYYY-MM-DDThh:mm:ss.sss[...]Z

The precision of the seconds is indefinite.

Further, to be able to access the service of OKAPI leave your sign in information in a file in the source directory and name it "okapi_acc". Username and password in the following order
- username
- password

Eventually leave the TLE, for which you want your passes to be calculated in the "tle" folder. Give the input files the ending ".tle". Any file that has this ending will be checked for TLEs. Any TLE that is found will be requested for the given time window.

Execute the routine via
```
LD_LIBRARY_PATH=../lib ./tle2passes
```

After finishing you'll find your passes written to "output.dat".
