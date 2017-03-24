To retrieve the most recent Maps API file, first determine what the
current version is by examing the JS returned by the standard script tag:

  http://maps.google.com/maps?file=api&v=2&key=abcdefg

This will name the current file, for example maps2.71.api.js.
Retrieve the file with wget:

  wget http://maps.google.com/mapfiles/maps2.71.api.js

minimize.gif  was created in photoshop by editing maximize.gif.
