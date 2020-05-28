
SET CLIENT_MIN_MESSAGES TO WARNING;

-- Function to install the example search definition. This function calls the
-- upsert_searchdef_content function that was created in gesearch_tables.sql.

CREATE OR REPLACE FUNCTION run_examplesearch_upsert()
RETURNS VOID AS $$
DECLARE
   searchDefName VARCHAR := 'ExampleSearch';
   searchDefContents VARCHAR := 
            '{"additional_config_param": null, "label": "Example search", "additional_query_param": "flyToFirstElement=true&displayKeys=location", "service_url": "/gesearch/ExampleSearch", "result_type": "KML", "html_transform_url": "about:blank", "kml_transform_url":  "about:blank", "suggest_server": "about:blank", "fields": [{"key": "q", "suggestion": "San Francisco neighborhoods", "label": null}]}';
BEGIN
 PERFORM upsert_searchdef_content(searchDefName, searchDefContents);
END;
$$ LANGUAGE plpgsql;

SELECT run_examplesearch_upsert();
