var points = [];
var marker = null;
var highlighted_marker = null;
var point_markers = [];

// Add a point to the points list.
function addPoint() {
  var lat = document.getElementById('txtLatitude').value;
  var pLat = parseFloat(lat);

  if (pLat.toString() != lat) {
    alert('Invalid latitude entered. Must be in range of -90 to 90');
    return;
  }

  if (pLat < -90 || pLat > 90) {
    alert('Invalid latitude entered. Must be in range of -90 to 90');
    return;
  }

  var lng = document.getElementById('txtLongitude').value;
  var pLong = parseFloat(lng);

  if (pLong.toString() != lng) {
    alert('Invalid longitude entered. Must be in range of -180 to 180');
    return;
  }

  if (pLong < -180 || pLong > 180) {
    alert('Invalid longitude entered. Must be in range of -180 to 180');
    return;
  }

  var level = document.getElementById('txtLevel').value;
  var pLevel = parseInt(level);

  if (pLevel.toString() != level) {
    alert('Invalid minimum level entered. Must be in range of 0 to 3');
    return;
  }

  if (pLevel < 0 || pLevel > 3) {
    alert('Invalid minimum level entered. Must be in range of 0 to 3');
    return;
  }

  createPoint(lat, lng, pLevel);
  createEncodings(false);
}

// Returns the index of the marker in the polyline.
function findMarkerIndex(point_marker) {
  var index = -1;

  for (var  i = 0; i < point_markers.length; ++i) {
    if (point_markers[i] == point_marker) {
      index = i;
      break;
    }
  }

  return index;
}

// Creates a point and adds it to both the polyline and the list.
function createPoint(lat, lng, pLevel) {
  addPointItem(lat, lng, pLevel);

  var newPoint = {
    Latitude: lat,
    Longitude: lng,
    Level: pLevel
  };

  points.push(newPoint);

  if (marker) {
    document.map.removeOverlay(marker);
    marker = null;
  }

  var point_marker = createPointMarker(new GLatLng(lat, lng), false);
  document.map.addOverlay(point_marker);
  point_markers.push(point_marker);
}

// Creates a marker representing a point in the polyline.
function createPointMarker(point, highlighted) {
  var clr = highlighted ? "yellow" : "blue";

  var point_marker = createMarker(point, clr);
  point_marker.enableDragging();

  GEvent.addListener(point_marker, "drag", function() {
    var index = findMarkerIndex(point_marker);

    if (index >= 0) {
      var nLat = point_marker.getPoint().lat();
      var nLng = point_marker.getPoint().lng();

      var pLevel = points[index].Level;

      var modifiedPoint = {
        Latitude: nLat,
        Longitude: nLng,
        Level: pLevel
      };

      points[index] = modifiedPoint;
      createEncodings(false);
      document.getElementById('pointList').options[index]
        = new Option('(' + nLat + ',' + nLng + ') Level: ' + pLevel, index);
    }
  });

  GEvent.addListener(point_marker, "click", function() {
    highlight(findMarkerIndex(point_marker));
  });

  return point_marker;
}

// Add an option to the points list with the specified information.
function addPointItem(lat, lng, pLevel) {
  var displayPoint = new Option('(' + lat + ',' + lng + ') Level: ' + pLevel,
                                points.length);
  document.getElementById('pointList').options.add(displayPoint);
}

// Highlights the point specified by index in both the map and the point list.
function highlight(index) {
  var pointList = document.getElementById('pointList');

  if (index < pointList.length) {
    pointList.selectedIndex = index;
  }

  if (point_markers[index] != null
      && point_markers[index] != highlighted_marker) {
    document.map.removeOverlay(point_markers[index]);
  }

  if (highlighted_marker != null) {
    var oldIndex = findMarkerIndex(highlighted_marker);
    document.map.removeOverlay(highlighted_marker);

    if (oldIndex != index) {
      point_markers[oldIndex]
        = createPointMarker(highlighted_marker.getPoint(), false);
      document.map.addOverlay(point_markers[oldIndex]);
    }
  }

  highlighted_marker = createPointMarker(point_markers[index].getPoint(),
                                         true);
  point_markers[index] = highlighted_marker;
  document.map.addOverlay(highlighted_marker);
}

// Encode a signed number in the encode format.
function encodeSignedNumber(num) {
  var sgn_num = num << 1;

  if (num < 0) {
    sgn_num = ~(sgn_num);
  }

  return(encodeNumber(sgn_num));
}

// Encode an unsigned number in the encode format.
function encodeNumber(num) {
  var encodeString = "";

  while (num >= 0x20) {
    encodeString += (String.fromCharCode((0x20 | (num & 0x1f)) + 63));
    num >>= 5;
  }

  encodeString += (String.fromCharCode(num + 63));
  return encodeString;
}

// Delete *all* the points from the polyline, with confirmation dialog before
// deletion.
function deleteAllPoints() {
  var deleteConfirm = confirm("Are you sure you want to remove all the points"
                              + " from this polyline?");

  if (deleteConfirm) {
    document.getElementById('pointList').options.length = 0;
    points = [];
    deleteAllMarkers();
    createEncodings();
  }
}

// Deletes all the markers for the points in the polyline
function deleteAllMarkers() {
  for(var i = 0; i < point_markers.length; ++i) {
    document.map.removeOverlay(point_markers[i]);
  }

  point_markers = [];
  highlighted_marker = null;
}

// Delete a point from the polyline.
function deletePoint() {
  if (points.length > 0) {
    var point_index = document.getElementById('pointList').selectedIndex;

    if (point_index >= 0 && point_index < points.length) {
      points.splice(point_index, 1);

      if (highlighted_marker == point_markers[point_index]) {
        highlighted_marker = null;
      }

      document.map.removeOverlay(point_markers[point_index]);
      point_markers.splice(point_index, 1);
      document.getElementById('pointList').options[point_index] = null;
      createEncodings();
    }

    if (points.length > 0) {
      if (point_index == 0) {
        point_index++;
      }

      highlight(point_index - 1);
    }
  }
}

// Try to encode an unsigned number. Used by the documentation.
function tryEncode() {
  var txtValue = document.getElementById('txtNumber').value;
  if (parseInt(txtValue).toString() == txtValue) {
    document.getElementById('cdeValue').innerHTML
      = encodeNumber(parseInt(txtValue));
  }else{
    document.getElementById('cdeValue').innerHTML = '(None)';
  }
}

// Try to encode a signed number. Used by the documentation.
function trySignEncode() {
  var txtValue = document.getElementById('txtSignNumber').value;
  if (parseInt(txtValue).toString() == txtValue) {
    document.getElementById('cdeSignValue').innerHTML
      = encodeSignedNumber(parseInt(txtValue));
  }else{
    document.getElementById('cdeSignValue').innerHTML = '(None)';
  }
}

// Create the encoded polyline and level strings. If moveMap is true
// move the map to the location of the first point in the polyline.
function createEncodings(moveMap) {
  var i = 0;

  var plat = 0;
  var plng = 0;

  var encoded_points = "";
  var encoded_levels = "";

  for(i = 0; i < points.length; ++i) {
    var point = points[i];
    var lat = point.Latitude;
    var lng = point.Longitude;
    var level = point.Level;

    var late5 = Math.round(lat * 1e5);
    var lnge5 = Math.round(lng * 1e5);

    dlat = late5 - plat;
    dlng = lnge5 - plng;

    plat = late5;
    plng = lnge5;

    encoded_points += encodeSignedNumber(dlat) + encodeSignedNumber(dlng);
    encoded_levels += encodeNumber(level);
  }

  // move if moveMap is true.
  if (moveMap) {
    document.map.setCenter(
        new GLatLng(points[0].Latitude, points[0].Longitude),
        document.map.getZoom());
  }

  document.getElementById('encodedLevels').value = encoded_levels;
  document.getElementById('encodedPolyline').value = encoded_points;

  if (document.overlay) {
    document.map.removeOverlay(document.overlay);
  }

  if (points.length > 1) {
    document.overlay = GPolyline.fromEncoded({color: "#0000FF",
                                              weight: 10,
                                              points: encoded_points,
                                              zoomFactor: 32,
                                              levels: encoded_levels,
                                              numLevels: 4
                                             });

    document.map.addOverlay(document.overlay);
  }
}

function centerMap() {
  var address = document.getElementById('txtAddress').value;

  if (address.length > 0) {
    var geocoder = new GClientGeocoder();

    geocoder.getLatLng(address,
      function(point) {
        if (!point) {
          alert('Address "' + address + '" not found');
        } else {
          document.map.setCenter(point, 13);
        }
      });
  }
}

// Decode an encoded polyline into a list of lat/lng tuples.
function decodeLine (encoded) {
  var len = encoded.length;
  var index = 0;
  var array = [];
  var lat = 0;
  var lng = 0;

  while (index < len) {
    var b;
    var shift = 0;
    var result = 0;
    do {
      b = encoded.charCodeAt(index++) - 63;
      result |= (b & 0x1f) << shift;
      shift += 5;
    } while (b >= 0x20);
    var dlat = ((result & 1) ? ~(result >> 1) : (result >> 1));
    lat += dlat;

    shift = 0;
    result = 0;
    do {
      b = encoded.charCodeAt(index++) - 63;
      result |= (b & 0x1f) << shift;
      shift += 5;
    } while (b >= 0x20);
    var dlng = ((result & 1) ? ~(result >> 1) : (result >> 1));
    lng += dlng;

    array.push([lat * 1e-5, lng * 1e-5]);
  }

  return array;
}

// Decode an encoded levels string into a list of levels.
function decodeLevels(encoded) {
  var levels = [];

  for (var pointIndex = 0; pointIndex < encoded.length; ++pointIndex) {
    var pointLevel = encoded.charCodeAt(pointIndex) - 63;
    levels.push(pointLevel);
  }

  return levels;
}

// Decode the supplied encoded polyline and levels.
function decode() {
  var encoded_points = document.getElementById('encodedPolyline').value;
  var encoded_levels = document.getElementById('encodedLevels').value;

  if (encoded_points.length==0 || encoded_levels.length==0) {
    return;
  }

  var enc_points = decodeLine(encoded_points);
  var enc_levels = decodeLevels(encoded_levels);

  if (enc_points.length==0 || enc_levels.length==0) {
    return;
  }

  if (enc_points.length != enc_levels.length) {
    alert('Point count and level count do not match');
    return;
  }

  deleteAllMarkers();
  document.getElementById('pointList').options.length = 0;
  points = [];

  for (var i = 0; i < enc_points.length; ++i) {
    createPoint(enc_points[i][0], enc_points[i][1], enc_levels[i]);
  }

  createEncodings(true);
}

function createMarker(point, color) {
  var f = new GIcon();
  f.image = "http://labs.google.com/ridefinder/images/mm_20_" + color
            + ".png";
  f.shadow = "http://labs.google.com/ridefinder/images/mm_20_shadow.png";
  f.iconSize = new GSize(12,20);
  f.shadowSize = new GSize(22,20);
  f.iconAnchor = new GPoint(6,20);
  f.infoWindowAnchor = new GPoint(6,1);
  f.infoShadowAnchor = new GPoint(13,13);

  newMarker = new GMarker(point,
    {icon: f,
     draggable: true});

  return newMarker;
}

// Create the Google Map to be used.
function createMap() {
  if (!GBrowserIsCompatible()) {
    alert('Your browser is not compatible with the Google Maps API');
    return;
  }

  document.map = new GMap2(document.getElementById("map_canvas"));
  document.map.setCenter(new GLatLng(37.4419, -122.1419), 13);
  document.map.addControl(new GSmallMapControl());
  document.map.addControl(new GMapTypeControl());

  GEvent.addListener(document.map, "click", function(overlay, point) {
    document.getElementById('txtLatitude').value = point.y;
    document.getElementById('txtLongitude').value = point.x;

    if (marker == null) {
      marker = createMarker(point, "green");
      marker.enableDragging();

      GEvent.addListener(marker, "drag", function() {
        document.getElementById('txtLatitude').value = marker.getPoint().y;
        document.getElementById('txtLongitude').value = marker.getPoint().x;
      });

      document.map.addOverlay(marker);
    } else {
      marker.setPoint(point);
    }
  });
}

// Move the map to the selected point in the point list.
function jumpToPoint() {
  var pointList = document.getElementById('pointList');
  if (pointList.selectedIndex >= 0) {
    var point = points[pointList.selectedIndex];
    document.map.setCenter(new GLatLng(point.Latitude, point.Longitude),
                           document.map.getZoom());
  }
}
