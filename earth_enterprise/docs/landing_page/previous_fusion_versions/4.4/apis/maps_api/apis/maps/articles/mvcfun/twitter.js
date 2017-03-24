var distanceWidget;
var map;
var geocodeTimer;
var profileMarkers = [];


function init() {
  var mapDiv = document.getElementById('map');
  map = new google.maps.Map(mapDiv, {
    center: new google.maps.LatLng(37.790234970864, -122.39031314844),
    zoom: 8,
    mapTypeId: google.maps.MapTypeId.ROADMAP
  });

  distanceWidget = new DistanceWidget({
    map: map,
    distance: 50, // Starting distance in km.
    maxDistance: 2500, // Twitter has a max distance of 2500km.
    color: '#000',
    activeColor: '#59b',
    sizerIcon: new google.maps.MarkerImage('resize-off.png'),
    activeSizerIcon: new google.maps.MarkerImage('resize.png')
  });

  google.maps.event.addListener(distanceWidget, 'distance_changed',
      updateDistance);

  google.maps.event.addListener(distanceWidget, 'position_changed',
      updatePosition);

  map.fitBounds(distanceWidget.get('bounds'));

  updateDistance();
  updatePosition();
  addActions();
}

function updatePosition() {
  if (geocodeTimer) {
    window.clearTimeout(geocodeTimer);
  }

  // Throttle the geo query so we don't hit the limit
  geocodeTimer = window.setTimeout(function() {
    reverseGeocodePosition();
  }, 200);
}

function reverseGeocodePosition() {
  var pos = distanceWidget.get('position');
  var geocoder = new google.maps.Geocoder();
  geocoder.geocode({'latLng': pos}, function(results, status) {
    if (status == google.maps.GeocoderStatus.OK) {
      if (results[1]) {
        $('#of').html('of ' + results[1].formatted_address);
        return;
      }
    }

    $('#of').html('of somewhere');
  });
}

function updateDistance() {
  var distance = distanceWidget.get('distance');
  $('#dist').html(distance.toFixed(2));
}

function addActions() {
  var s = $('#s').submit(search);

  $('#close').click(function() {
    $('#cols').removeClass('has-cols');
    google.maps.event.trigger(map, 'resize');
    map.fitBounds(distanceWidget.get('bounds'));
    $('#results-wrapper').hide();

    return false;
  });
}

function search(e) {
  e.preventDefault();
  var q = $('#q').val();
  if (q == '') {
    return false;
  }

  var d = distanceWidget.get('distance');
  var p = distanceWidget.get('position');

  var url = 'http://search.twitter.com/search.json?callback=addResults' +
    '&rrp=100&q=' + escape(q) + '&geocode=' + escape(p.lat() + ',' + p.lng() +
    ',' + d + 'km');

  clearMarkers();

  $.getScript(url);

  $('#results').html('Searching...');
  var cols = $('#cols');
  if (!cols.hasClass('has-cols')) {
    $('#cols').addClass('has-cols');
    google.maps.event.trigger(map, 'resize');
    map.fitBounds(distanceWidget.get('bounds'));
  }
}

function clearMarkers() {
  for (var i = 0, marker; marker = profileMarkers[i]; i++) {
    marker.setMap(null);
  }
}

function addResults(json) {
  var results = $('#results');
  results.innerHTML = '';
  html = [];
  if (json.results && json.results.length) {
    for (var i = 0, tweet; tweet = json.results[i]; i++) {
      var from = tweet.from_user;
      var profileImageUrl = tweet.profile_image_url;
      var loc = tweet.location;

      // Check if the location matches a latlng
      var point = loc.match(/-?\d+\.\d+/g);

      if (point && point.length == 2) {
        var image = new google.maps.MarkerImage(profileImageUrl,
          new google.maps.Size(48, 48),
          new google.maps.Point(0, 0),
          new google.maps.Point(24, 24),
          new google.maps.Size(24, 24));

        var pos = new google.maps.LatLng(parseFloat(point[0], 10),
            parseFloat(point[1], 10));

        var marker = new google.maps.Marker({
          map: map,
          position: pos,
          icon: image,
          zIndex: 10
        });

        profileMarkers.push(marker);
      }

      html.push('<div class="tweet"><span class="thumb">');
      html.push('<a href="http://twitter.com/' + from + '">');
      html.push('<img src="' + profileImageUrl + '"/></a></span>');
      html.push('<div class="body"<a href="http://twitter.com/' + from);
      html.push('"></a>');
      html.push(tweet.text);
      html.push('</div><div class="body location">From: ' + from);
      html.push(', near ' + loc);
      html.push('</div></div>');
    }
  } else {
    html.push('<div class="no-tweets">No tweets found.</div>');
  }

  $(results).html(html.join(''));
  $('#results-wrapper').show();
}

google.maps.event.addDomListener(window, 'load', init);
