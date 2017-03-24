// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the Licens
//

package com.google.earth.search.plugin;

import java.io.IOException;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

import javax.servlet.ServletContext;

import com.google.earth.search.api.GeomMetadata;
import com.google.earth.search.api.KmlPluginPreferences;
import com.google.earth.search.api.PluginPreferences;
import com.google.earth.search.api.SearchContainer;
import com.google.earth.search.api.KmlSearchPlugin;
import com.google.earth.search.api.SearchRequest;
import com.google.earth.search.api.SearchResponse;
import com.vividsolutions.jts.geom.Geometry;
import com.vividsolutions.jts.geom.GeometryCollection;
import com.vividsolutions.jts.geom.GeometryFactory;

/**
 * This is an Example Plugin implementation that demonstrates how one can
 * construct and query a spatial database based on URL search string, extract
 * geometries from the result, associate various styles with them, and return
 * them as part of a SearchResponse
 */
public class ExamplePlugin extends KmlSearchPlugin {

  private static final String NEIGHBORHOOD_STYLE = "neighborhood_style";
  private static final String NEIGHBORHOOD = "neighborhood";
  Connection conn = null;

  /**
   * Called by the Plugin Factory once the class is loaded. Here we can perform
   * some of our init checks (check database connection etc.). This method has
   * to return true in order for the Plugin to be calleable by an Adapter.
   */
  public boolean init(ServletContext context) {

    // Assume the connection will be successful
    boolean status = true;

    // Define a connection string to the PostgreSQL database, 'ExampleDatabase'
    String url = "jdbc:postgresql://localhost/searchexample";
    Properties props = new Properties();
    props.setProperty("user", "geuser");
    props.setProperty("password", "");

    // Try and connect
    try {
      conn = DriverManager.getConnection(url, props);
    } catch (SQLException e) {
      status = false;
    }

    return status;
  }

  public boolean isRequestValid(SearchRequest searchRequest) {
    Map<String, String> params = searchRequest.getSearchRequestItems();
    boolean isValid = params != null && params.containsKey(NEIGHBORHOOD);
    return isValid;
  }

  /**
   * Called by the Adapter to begin the search phase
   * 
   * @param searchContainer The SearchContainer object containing that contains
   *        the SearchRequest and one or many SearchResponses
   */
  public boolean doSearch(SearchContainer searchContainer) {

    boolean status = true;
    KmlPluginPreferences preferences =
      ExamplePluginPreferences.getDefaultPreferences(
          searchContainer.getSearchRequest().getSearchRequestItems());

    SearchResponse exampleSearchResponse = new SearchResponse();
    exampleSearchResponse.setDataStoreName("Example Database");

    // Attempt to extract the search term 'neighborhood' from the query string
    if (searchContainer.getSearchRequest().contains(NEIGHBORHOOD)) {

      String neighborhoodSearchTerm =
          searchContainer.getSearchRequest().getItem(NEIGHBORHOOD);

      // Add the search string to the response object for KML diplay
      exampleSearchResponse.setResponseSearchTerm(neighborhoodSearchTerm);

      String exampleSqlQuery =
          "SELECT Encode(AsBinary(the_geom, 'XDR'), 'base64') AS the_geom, " +
          "Area(the_geom), Perimeter(the_geom), sfar_distr, nbrhood from " +
          "san_francisco_neighborhoods WHERE lower(nbrhood) like ?";
      PreparedStatement prepStmnt = null;
      ResultSet results = null;

      // Create our SQL prepared stament and execute it, handling the case where
      // the execution fails
      try {

        prepStmnt = conn.prepareStatement(exampleSqlQuery);
        prepStmnt
            .setString(1, "%" + neighborhoodSearchTerm.toLowerCase() + "%");
        results = prepStmnt.executeQuery();

      } catch (SQLException e) {

        status = false;
        exampleSearchResponse.setSuccess(false);
        exampleSearchResponse.setResponseErrorMessage("DB Query Error: "
            + e.getMessage());

      }

      if (status) {

        // Iterate through our results, constructing JTS geometries for each and
        // adding them to our SearchResponse object
        try {

          while (results.next()) {

            // Create the HashMap we'll use to populate with results
            HashMap<String, Object> searchResult =
                new HashMap<String, Object>();
            // Create an instance of a GeomMetadata
            GeomMetadata geomMeta = new GeomMetadata();

            // Add name meta block
            geomMeta.name = results.getString("nbrhood");
            geomMeta.snippet = results.getString("sfar_distr");
            geomMeta.description =
                "The total area in decimal degrees of "
                    + results.getString("nbrhood") + " is: "
                    + results.getString("area") + "<br>";
            geomMeta.description +=
                "The total perimeter in decimal degrees of "
                    + results.getString("nbrhood") + " is: "
                    + results.getString("perimeter");

            // Create a JTS Geometry out of our base64 encoded, PostGIS XDR's
            // WKB
            Geometry geom =
                geometryFromWKBResultSet(results.getBytes("the_geom"));

            // Add our meta data block to the geometry
            geom.setUserData(geomMeta);

            // Create a JTS GeometryCollection and place our new Geometry in it.
            Geometry[] geoms = new Geometry[1];
            geoms[0] = geom;
            GeometryCollection geomCollection =
                new GeometryFactory().createGeometryCollection(geoms);

            // Add our JTS Geometry to the HashMap
            searchResult.put("geometry", geomCollection);

            // Add our default style
            searchResult.put("styleID", NEIGHBORHOOD_STYLE);

            // Add to the SearchResponse and we're done
            exampleSearchResponse.addSearchResponse(searchResult);
            // Define a reasonable style for our results, and add it to the
            // SearchResponse Object
            setStyle(exampleSearchResponse, preferences, NEIGHBORHOOD_STYLE);

          }

          // db cleanup
          results.close();


        } catch (SQLException e) {

          status = false;
          exampleSearchResponse.setSuccess(false);
          exampleSearchResponse.setResponseErrorMessage("DB Results Error: "
              + e.getMessage());

        } catch (IOException e) {

          status = false;
          exampleSearchResponse.setSuccess(false);
          exampleSearchResponse
              .setResponseErrorMessage("Geometry parsing error: "
                  + e.getMessage());

        }

      }

      // The search term 'neighborhood' wasn't found in the search request
    } else {
      status = false;
      exampleSearchResponse.setSuccess(false);
      exampleSearchResponse
          .setResponseErrorMessage("Unable to interpret search request");
    }

    searchContainer.addSearchResponse(exampleSearchResponse);
    return status;
  }

  /**
   * Generates a JTS Geometry type from the WKB returned in the db query
   * 
   * @param bytes bytes array returned in the search query
   * @return a JTS geometry derived from the PostGIS geometry
   * @throws IOException
   */
  private Geometry geometryFromWKBResultSet(byte[] bytes) throws IOException {

    // Instantiate a WKB Reader object
    org.geotools.data.postgis.attributeio.WKBReader wkbReader =
        new org.geotools.data.postgis.attributeio.WKBReader();
    // Decode the result
    byte[] b = org.geotools.data.postgis.attributeio.Base64.decode(bytes);
    // WKB->JTS Geom
    Geometry wkbGeometry = wkbReader.read(b);
    // Return new JTS geometry
    return new GeometryFactory().createGeometry(wkbGeometry);
  }

  /**
   * Called by the PluginFactor when right before it deletes the Plugin from the Tomcat context
   */
  public void close() {
  }

  @Override
  public PluginPreferences getDefaultPreferences(Map<String, String> overrides) {
    PluginPreferences preferences = new ExamplePluginPreferences();
    preferences.Override(overrides);
    return preferences;
  }
}
