TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on debug

LIBS	+= -Wl,-rpath,$(GSTLIBDIR) -L$(GSTLIBDIR) -lgst

DEFINES	+= QT_NO_ASCII_CAST

INCLUDEPATH	+= ../include

HEADERS	+= GfxView.h \
	ProjectManager.h \
	MainWindow.h \
	LayerProperties.h \
	SelectionRules.h \
	QueryRules.h \
	SiteIcons.h \
	Preferences.h \
	LabelFormat.h \
	ProjectSettings.h \
	ProjectOverview.h \
	SelectionView.h \
	SourceFileDialog.h \
	GlobalFusion.h \
	PlacemarkManager.h \
	BulkSource.h \
	IconManager.h \
	SourceProperties.h \
	AssetManager.h \
	AssetProperties.h \
	ProjectLayerView.h \
	AssetIconView.h \
	AssetTableView.h \
	DataViewTable.h \
	LocaleDetails.h

SOURCES	+= GfxView.cpp \
	main.cpp \
	ProjectManager.cpp \
	MainWindow.cpp \
	LayerProperties.cpp \
	SelectionRules.cpp \
	QueryRules.cpp \
	ProjectBuilder.cpp \
	SiteIcons.cpp \
	Preferences.cpp \
	LabelFormat.cpp \
	ProjectSettings.cpp \
	ProjectOverview.cpp \
	SelectionView.cpp \
	SourceFileDialog.cpp \
	GlobalFusion.cpp \
	PlacemarkManager.cpp \
	BulkSource.cpp \
	IconManager.cpp \
	SourceProperties.cpp \
	AssetManager.cpp \
	AssetProperties.cpp \
	ProjectLayerView.cpp \
	AssetIconView.cpp \
	AssetTableView.cpp \
	DataViewTable.cpp \
	LocaleDetails.cpp

FORMS	= mainwindowbase.ui \
	tableviewbase.ui \
	layerpropertiesbase.ui \
	selectionrulesbase.ui \
	siteiconsbase.ui \
	preferencesbase.ui \
	labelformatbase.ui \
	generictextbase.ui \
	placemarkmanagerbase.ui \
	selectionviewbase.ui \
	progressbase.ui \
	iconmanagerbase.ui \
	assetmanagerbase.ui \
	systemmanagerbase.ui \
	servercombinationeditbase.ui \
	providermanagerbase.ui \
	placemarkeditbase.ui \
	assetpropertiesbase.ui \
	imageproductbase.ui \
	assetversionpropertiesbase.ui \
	assetlogbase.ui \
	layergrouppropertiesbase.ui \
	newlayergroupbase.ui \
	geographictransformbase.ui \
	spatialreferencesystembase.ui \
	rasterpreviewpropertiesbase.ui \
	rasterlayerpropertiesbase.ui \
	assetchooserbase.ui \
	rasterpropertiesbase.ui \
	srsdetailsbase.ui \
	advancedlabeloptionsbase.ui \
	aboutfusionbase.ui \
	textimportbase.ui \
	objectdetailbase.ui \
	featureeditorbase.ui \
	newfeaturebase.ui \
	labelmanagerbase.ui \
	scripteditorbase.ui \
	thematicfilterbase.ui \
	newassetbase.ui \
	assetnotesbase.ui \
	layerlegendbase.ui \
	labelpropertiesbase.ui \
	vectorassetwidgetbase.ui \
	rasterassetwidgetbase.ui \
	textstylebase.ui \
	databasewidgetbase.ui \
	localemanagerbase.ui \
	projectwidgetbase.ui \
	maplayerwidgetbase.ui \
	searchtabdetailsbase.ui \
	mapdatabasewidgetbase.ui \
	searchfielddialogbase.ui \
	publisherbase.ui \
	providereditbase.ui \
	publishdatabasedialogbase.ui \
	authdialogbase.ui \
	balloonstyletextbase.ui \
	vectorlayerwidgetbase.ui \
	mercatormapdatabasewidgetbase.ui

IMAGES	= images/filenew \
	images/fileopen \
	images/print \
	images/undo \
	images/redo \
	images/editcut \
	images/editcopy \
	images/editpaste \
	images/searchfind \
	grids.xpm \
	select.xpm \
	zoombox.xpm \
	zoomdrag.xpm \
	hand.xpm \
	images/project_open.png \
	images/project_new.png \
	images/lock.png \
	images/unlock.png \
	images/keyhole_logo.png \
	images/folder.xpm \
	images/fileopen_1.png \
	images/window_fullscreen.png \
	images/window_nofullscreen.png \
	images/exit.png \
	images/folder_new.png \
	images/hi22-action-make_kdevelop.png \
	images/project_open.png \
	images/make_kdevelop.png \
	images/project.png \
	images/editdelete.png \
	images/up.png \
	images/down.png \
	images/project_save.png \
	images/stop.png \
	type_image.xpm \
	zoombox.xpm \
	images/type_vector.xpm \
	images/type_terrain.xpm \
	images/vector_asset.png \
	images/imagery_asset.png \
	images/terrain_asset.png \
	images/failed_asset.png \
	images/shield_type1.png \
	images/shield_type2.png \
	images/shield_type3.png \
	images/history.png \
	images/delete.png \
	images/folder_open.xpm \
	images/folder_closed.xpm \
	images/vector_project.png \
	images/terrain_project.png \
	images/imagery_project.png \
	images/cdtoparent.xpm \
	images/newfolder.xpm \
	images/open_folder.xpm \
	images/closed_folder.xpm \
	images/database.png \
	images/zoombox_cursor.png \
	images/zoombox_cursor_mask.png \
	images/1downarrow.png \
	images/up_16x16.png \
	images/down_16x16.png \
	images/primtype_point.png \
	images/edit_tool.png \
	images/google_earth_logo.png \
	images/kml_project.png \
	images/map_layer.png \
	images/map_project.png \
	images/left.png \
	images/right.png \
	images/map_database.png \
	images/fileclose.png \
	images/notes.png \
	images/filesaveas.png \
	images/filesave.png \
	images/vector_layer.png \
	images/timemachine-on.png \
	images/timemachine-off.png \
	images/imagery_asset_mercator.png \
	images/imagery_project_mercator.png \
	images/map_database_mercator.png

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}
TRANSLATIONS = fusion_jp.ts \
		fusion_es.ts
DEFAULTCODEC = ISO-8859-5


# Override default warning flags to ignore unused functions.
# The Qt tool 'moc' generates code with lots of them. :-(
QMAKE_CFLAGS_WARN_ON = -Wall -W -Wno-unused-function
QMAKE_CXXFLAGS_WARN_ON = -Wall -W -Wno-unused-function

# Override the default flags to get something sane
QMAKE_CFLAGS_RELEASE   = -O2 -march=i686 -DNDEBUG
QMAKE_CXXFLAGS_RELEASE = -O2 -march=i686 -DNDEBUG
QMAKE_CFLAGS_DEBUG     = -g -march=i686
QMAKE_CXXFLAGS_DEBUG   = -g -march=i686
