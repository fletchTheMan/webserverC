This is a simple HTTP server that I made. It is multithreaded and so should run well but since it will only
serve simple html css and pngs it does not need to be. *This is not intended to be run in production and is just a example of what
I can do*
## Building
This requires linux to run. It might run on BSD or other UNIX but I don't know and will not try
Requirements
- gcc
- linux
That should be it for the requirements.
Since it uses makefile run `make` to build.
### Example
to build the example use `make move` to then move the html files and other things to the main folder
and then run the program, the first page should be displayed with an icon and some css the second page should
not have any css but some text
## Running
After building the project with the `make` command then run it with the command `./webserver`
make sure to have a html file in the same folder as it. There are some examples that can be moved with `mv examples/* ./`
