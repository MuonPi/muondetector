import QtQuick 2.7
import QtQuick.Controls 2.5
import QtLocation 5.7
import QtPositioning 5.7
import QtQuick.Layouts 1.0
import "qrc:/qml/places/items"
//import "qml/places/items"
/*
import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12
*/
/*
Plugin {
        id: mapPlugin
        name: "osm" // "mapboxgl", "esri", ...
        // specify plugin parameters if necessary
        // PluginParameter {
        //     name:
        //     value:
        // }
}

function setCircle(lon,lat,hAcc)
{
    map.onCoordsReceived(lon,lat,hAcc)
}

Map {
    id: map
    anchors.fill: parent
    plugin: mapPlugin
    center: QtPositioning.coordinate(50.569212, 8.673828) // 2. Physikalisches Institut Gießen
    zoomLevel: 14
    function onCoordsReceived(lon,lat,hAcc)
    {
        circle.center.longitude = lon
        circle.center.latitude = lat
        circle.radius = hAcc
    }
    MapCircle {
        id: circle
        center: parent.center
        radius: 0.0
        border.width: 2
    }
}*/

Item{
    id: rootItem
    property variant map: Map
    property variant parameters
    property variant searchLocation: map ? map.center : QtPositioning.coordinate()
    property variant searchRegion: QtPositioning.circle(QtPositioning.coordinate(50.569212, 8.673828),1000)
    property variant searchRegionItem
    property Plugin favoritesPlugin
    //anchors.fill:  parent

    function getPlugins() {
        var plugin = Qt.createQmlObject('import QtLocation 5.3; Plugin {}', rootItem);
        var myArray = new Array;
        for (var i = 0; i < plugin.availableServiceProviders.length; i++) {
            var tempPlugin = Qt.createQmlObject ('import QtLocation 5.3; Plugin {name: "' + plugin.availableServiceProviders[i]+ '"}', rootItem)

            if (tempPlugin.supportsPlaces() && tempPlugin.supportsMapping() )
                myArray.push(tempPlugin.name)
        }
        myArray.sort()
        return myArray;
    }

    function initializeProviders(pluginParameters)
    {
        /*var parameters = new Array;
        for (var prop in pluginParameters) {
            var parameter = Qt.createQmlObject('import QtLocation 5.3; PluginParameter{ name: "'+ prop + '"; value: "' + pluginParameters[prop]+'"}',rootItem)
            parameters.push(parameter)
        }
        rootItem.parameters = parameters
        var plugins = getPlugins()
        mainMenu.providerMenu.createMenu(plugins)
        for (var i = 0; i<plugins.length; i++) {
            if (plugins[i] === "osm")
                mainMenu.selectProvider(plugins[i])
        }*/
        createMap("osm")
    }

    function createMap(provider) {
        var plugin;
        if (parameters && parameters.length>0)
            plugin = Qt.createQmlObject ('import QtLocation 5.3; Plugin{ name:"' + provider + '"; parameters: rootItem.parameters}', rootItem)
        else
            plugin = Qt.createQmlObject ('import QtLocation 5.3; Plugin{ name:"' + provider + '"}', rootItem)

        if (map){
            //map.destroy();
        }
        map = mapComponent.createObject(page);
        map.plugin = plugin;
        map.zoomLevel = (map.maximumZoomLevel - map.minimumZoomLevel)/2
        categoryModel.plugin = plugin;
        categoryModel.update();
        placeSearchModel.plugin = plugin;
        suggestionModel.plugin = plugin;
    }
    visible: true

   /* MainMenu {
        id: mainMenu
        onSelectProvider: {
            stackView.pop(page)
            for (var i = 0; i < providerMenu.count; i++) {
                providerMenu.itemAt(i).checked = providerMenu.itemAt(i).text === providerName
            }

            createMap(providerName)
            if (map.error === Map.NoError) {
                settingsMenu.createMenu(map);
            } else {
                settingsMenu.clear();
            }
        }
        onSelectSetting: {
            stackView.pop({tem:page,immediate: true})
            switch (setting) {
            case "searchCenter":
                stackView.push({ item: Qt.resolvedUrl("qrc:/qml/places/forms/SearchCenter.qml") ,
                                   properties: { "coordinate": map.center}})
                stackView.currentItem.changeSearchCenter.connect(stackView.changeSearchCenter)
                stackView.currentItem.closeForm.connect(stackView.closeForm)
                break
            case "searchBoundingBox":
                stackView.push({ item: Qt.resolvedUrl("qrc:/qml/places/forms/SearchBoundingBox.qml") ,
                                   properties: { "searchRegion": searchRegion}})
                stackView.currentItem.changeSearchBoundingBox.connect(stackView.changeSearchBoundingBox)
                stackView.currentItem.closeForm.connect(stackView.closeForm)
                break
            case "searchBoundingCircle":
                stackView.push({ item: Qt.resolvedUrl("qrc:/qml/places/forms/SearchBoundingCircle.qml") ,
                                   properties: { "searchRegion": searchRegion}})
                stackView.currentItem.changeSearchBoundingCircle.connect(stackView.changeSearchBoundingCircle)
                stackView.currentItem.closeForm.connect(stackView.closeForm)
                break
            case "earchOptions":
                stackView.push({ item: Qt.resolvedUrl("qrc:/qml/places/forms/SearchOptions.qml") ,
                                   properties: { "plugin": map.plugin,
                                       "model": placeSearchModel}})
                stackView.currentItem.changeSearchSettings.connect(stackView.changeSearchSettings)
                stackView.currentItem.closeForm.connect(stackView.closeForm)
                break
            default:
                console.log("Unsupported setting !")
            }
        }
    }*/

    /*Rectangle{
        id: barRectangle
        anchors.fill: parent
        width: rootItem.width
        height: 300
        SearchBar{
            id: bar
            searchBarVisbile: true
            visible: true
        }
    }
*/

    //! [PlaceSearchSuggestionModel search text changed 1]
    SearchBar {
        id: searchBar
    //! [PlaceSearchSuggestionModel search text changed 1]
        searchBarVisbile: stackView.depth > 1 &&
                          stackView.currentItem &&
                          stackView.currentItem.objectName != "suggestionView" ? false : true
        onShowCategories: {
            if (map && map.plugin) {
                stackView.pop({tem:page,immediate: true})
                stackView.enterCategory()
            }
        }
        onGoBack: stackView.pop()
    //! [PlaceSearchSuggestionModel search text changed 2]
        onSearchTextChanged: {
            if (searchText.length >= 3 && suggestionModel != null) {
                suggestionModel.searchTerm = searchText;
                suggestionModel.update();
            }
        }
    //! [PlaceSearchSuggestionModel search text changed 2]
        onDoSearch: {
            if (searchText.length > 0)
                placeSearchModel.searchForText(searchText);
        }
        onShowMap: stackView.pop(page)
    //! [PlaceSearchSuggestionModel search text changed 3]
    }
    //! [PlaceSearchSuggestionModel search text changed 3]


    StackView {
        id: stackView
        function showMessage(title,message,backPage)
        {
            push({ item: Qt.resolvedUrl("qrc:/qml/places/forms/Message.qml") ,
                     properties: {
                         "title" : title,
                         "message" : message,
                         "backPage" : backPage
                     }})
            currentItem.closeForm.connect(closeMessage)
        }

        function closeMessage(backPage)
        {
            pop(backPage)
        }

        function closeForm()
        {
            pop(page)
        }

        function enterCategory(index)
        {
            push({ item: Qt.resolvedUrl("qrc:/qml/places/views/CategoryView.qml") ,
                     properties: { "categoryModel": categoryModel,
                         "rootIndex" : index
                     }})
            currentItem.showSubcategories.connect(stackView.enterCategory)
            currentItem.searchCategory.connect(placeSearchModel.searchForCategory)
        }

        function showSuggestions()
        {
            if (currentItem.objectName != "suggestionView") {
                stackView.pop(page)
                push({ item: Qt.resolvedUrl("qrc:/qml/places/views/SuggestionView.qml") ,
                         properties: { "suggestionModel": suggestionModel }
                     })
                currentItem.objectName = "suggestionView"
                currentItem.suggestionSelected.connect(searchBar.showSearch)
                currentItem.suggestionSelected.connect(placeSearchModel.searchForText)
            }
        }

        function showPlaces()
        {
            if (currentItem.objectName != "searchResultView") {
                stackView.pop({tem:page,immediate: true})
                push({ item: Qt.resolvedUrl("qrc:/qml/places/views/SearchResultView.qml") ,
                         properties: { "placeSearchModel": placeSearchModel }
                     })
                currentItem.showPlaceDetails.connect(showPlaceDatails)
                currentItem.showMap.connect(searchBar.showMap)
                currentItem.objectName = "searchResultView"
            }
        }

        function showPlaceDatails(place, distance)
        {
            push({ item: Qt.resolvedUrl("qrc:/qml/places/forms/PlaceDetails.qml") ,
                     properties: { "place": place,
                         "distanceToPlace": distance }
                 })
            currentItem.searchForSimilar.connect(searchForSimilar)
            currentItem.showReviews.connect(showReviews)
            currentItem.showEditorials.connect(showEditorials)
            currentItem.showImages.connect(showImages)
        }

        function showEditorials(place)
        {
            push({ item: Qt.resolvedUrl("qrc:/qml/places/views/EditorialView.qml") ,
                     properties: { "place": place }
                 })
            currentItem.showEditorial.connect(showEditorial)
        }

        function showReviews(place)
        {
            push({ item: Qt.resolvedUrl("qrc:/qml/places/views/ReviewView.qml") ,
                     properties: { "place": place }
                 })
            currentItem.showReview.connect(showReview)
        }

        function showImages(place)
        {
            push({ item: Qt.resolvedUrl("qrc:/qml/places/views/ImageView.qml") ,
                     properties: { "place": place }
                 })
        }

        function showEditorial(editorial)
        {
            push({ item: Qt.resolvedUrl("qrc:/qml/places/views/EditorialPage.qml") ,
                     properties: { "editorial": editorial }
                 })
        }

        function showReview(review)
        {
            push({ item: Qt.resolvedUrl("qrc:/qml/places/views/ReviewPage.qml") ,
                     properties: { "review": review }
                 })
        }

        function changeSearchCenter(coordinate)
        {
            stackView.pop(page)
            map.center = coordinate;
            if (searchRegionItem) {
                map.removeMapItem(searchRegionItem);
                searchRegionItem.destroy();
            }
        }

        function changeSearchBoundingBox(coordinate,widthDeg,heightDeg)
        {
            stackView.pop(page)
            map.center = coordinate
            searchRegion = QtPositioning.rectangle(map.center, widthDeg, heightDeg)
            if (searchRegionItem) {
                map.removeMapItem(searchRegionItem);
                searchRegionItem.destroy();
            }
            searchRegionItem = Qt.createQmlObject('import QtLocation 5.3; MapRectangle { color: "#46a2da"; border.color: "#190a33"; border.width: 2; opacity: 0.25 }', page, "MapRectangle");
            searchRegionItem.topLeft = searchRegion.topLeft;
            searchRegionItem.bottomRight = searchRegion.bottomRight;
            map.addMapItem(searchRegionItem);
        }

        function changeSearchBoundingCircle(coordinate,radius)
        {
            stackView.pop(page)
            map.center = coordinate;
            searchRegion = QtPositioning.circle(coordinate, radius)

            if (searchRegionItem) {
                map.removeMapItem(searchRegionItem);
                searchRegionItem.destroy();
            }
            searchRegionItem = Qt.createQmlObject('import QtLocation 5.3; MapCircle { color: "#46a2da"; border.color: "#190a33"; border.width: 2; opacity: 0.25 }', page, "MapRectangle");
            searchRegionItem.center = searchRegion.center;
            searchRegionItem.radius = searchRegion.radius;
            map.addMapItem(searchRegionItem);
        }

        function changeSearchSettings(orderByDistance, orderByName, locales)
        {
            stackView.pop(page)
            /*if (isFavoritesEnabled) {
                if (favoritesPlugin == null)
                    favoritesPlugin = Qt.createQmlObject('import QtLocation 5.3; Plugin { name: "places_jsondb" }', page);
                favoritesPlugin.parameters = pluginParametersFromMap(pluginParameters);
                placeSearchModel.favoritesPlugin = favoritesPlugin;
            } else {
                placeSearchModel.favoritesPlugin = null;
            }*/

            placeSearchModel.favoritesPlugin = null;

            placeSearchModel.relevanceHint = orderByDistance ? PlaceSearchModel.DistanceHint :
                                                               orderByName ? PlaceSearchModel.LexicalPlaceNameHint :
                                                                             PlaceSearchModel.UnspecifiedHint;
            map.plugin.locales = locales.split(Qt.locale().groupSeparator);
        }

        //! [PlaceRecommendationModel search]
        function searchForSimilar(place) {
            stackView.pop(page)
            searchBar.showSearch(place.name)
            placeSearchModel.searchForRecommendations(place.placeId);
        }
        //! [PlaceRecommendationModel search]

        anchors.fill: parent
        focus: true
        initialItem:  Item {
            id: page

            //! [PlaceSearchModel model]
            PlaceSearchModel {
                id: placeSearchModel
                searchArea: searchRegion

                function searchForCategory(category) {
                    searchTerm = "";
                    categories = category;
                    recommendationId = "";
                    searchArea = searchRegion
                    limit = -1;
                    update();
                }

                function searchForText(text) {
                    searchTerm = text;
                    categories = null;
                    recommendationId = "";
                    searchArea = searchRegion
                    limit = -1;
                    update();
                }

                function searchForRecommendations(placeId) {
                    searchTerm = "";
                    categories = null;
                    recommendationId = placeId;
                    searchArea = null;
                    limit = -1;
                    update();
                }

                onStatusChanged: {
                    switch (status) {
                    case PlaceSearchModel.Ready:
                        if (count > 0)
                            stackView.showPlaces()
                        else
                            stackView.showMessage(qsTr("Search Place Error"),qsTr("Place not found !"))
                        break;
                    case PlaceSearchModel.Error:
                        stackView.showMessage(qsTr("Search Place Error"),errorString())
                        break;
                    }
                }
            }
            //! [PlaceSearchModel model]

            //! [PlaceSearchSuggestionModel model]
            PlaceSearchSuggestionModel {
                id: suggestionModel
                searchArea: searchRegion

                onStatusChanged: {
                    if (status == PlaceSearchSuggestionModel.Ready)
                        stackView.showSuggestions()
                }
            }
            //! [PlaceSearchSuggestionModel model]

            //! [CategoryModel model]
            CategoryModel {
                id: categoryModel
                hierarchical: true
            }
            //! [CategoryModel model]

            Component {
                id: mapComponent

                MapComponent {
                    width: page.width
                    height: page.height

                    onErrorChanged: {
                        if (map.error !== Map.NoError) {
                            var title = qsTr("ProviderError");
                            var message =  map.errorString + "<br/><br/><b>" + qsTr("Try to select other provider") + "</b>";
                            if (map.error === Map.MissingRequiredParameterError)
                                message += "<br/>" + qsTr("or see") + " \'mapviewer --help\' "
                                        + qsTr("how to pass plugin parameters.");
                            stackView.showMessage(title,message);
                        }
                    }

                    MapItemView {
                        model: placeSearchModel
                        delegate: MapQuickItem {
                            coordinate: model.type === PlaceSearchModel.PlaceResult ? place.location.coordinate : QtPositioning.coordinate()

                            visible: model.type === PlaceSearchModel.PlaceResult

                            anchorPoint.x: image.width * 0.28
                            anchorPoint.y: image.height

                            sourceItem: Image {
                                id: image
                                source: "qrc:/qml/places/resources/marker.png"
                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: stackView.showPlaceDatails(model.place,model.distance)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        color: "white"
        opacity: busyIndicator.running ? 0.8 : 0
        anchors.fill: parent
        Behavior on opacity { NumberAnimation{} }
    }
    BusyIndicator {
        id: busyIndicator
        anchors.centerIn: parent
        running: placeSearchModel.status == PlaceSearchModel.Loading ||
                 categoryModel.status === CategoryModel.Loading
    }
}
