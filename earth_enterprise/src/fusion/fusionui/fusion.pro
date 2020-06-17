TEMPLATE	= app
LANGUAGE	= C++

CONFIG	+= qt warn_on debug

LIBS	+= -Wl,-rpath,$(GSTLIBDIR) -L$(GSTLIBDIR) -lgst

DEFINES	+= QT_NO_ASCII_CAST

INCLUDEPATH	+= ../include

QT += qt3support

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

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}

RESOURCES = resources.qrc

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
