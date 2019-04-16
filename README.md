                 MSU projects
# HTTP-server

Actual HTTP-server that can respond to GET, HEAD, POST requests (POST requests for special commands)

## Getting started

Modified to be actual HTTP server with responds to **GET** requests, **HEAD** requests.

### Prerequisites

boost 1.68.0

MS Visual Studio 2015

Netcat *for testing* and browser

### Special commands for POST requests

As of 14 April 2019 the project has special POST requests' support. Special language is following:

- delpage page_name					          *deletes page with name page_name*

- delpar page_name par_number			    *deletes paragraph #par_number in page page_name*

- addpar page_name par_number text	  *adds text to paragraph #par_number in page page_name*

## Running the tests

As it is made on boost in Windows, no test scripts are made. But tester can run following commands by hands:

//compile and run project

```
nc.exe localhost 80
POST addpar doc.html 3 sometext
```

-> As a result the text "sometext" will be added to 3rd paragraph

```
nc.exe localhost 80
POST delpar doc.html 2
```

-> As a result paragraph #2 will be deleted

```
nc.exe localhost 80
POST delpage doc.html
```

-> As a result file doc.html will be deleted from filesystem

## Acknowledgements

Derived from simple echo server by Christopher M. Kohlhoff

## License

This project is licensed under the MIT License

## Author

Alisher Ikramov
