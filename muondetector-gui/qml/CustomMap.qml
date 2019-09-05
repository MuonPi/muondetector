import QtQuick 2.2
import QtLocation 5.7
import QtQuick.Layouts 1.0
import QtPositioning 5.0
import "qrc:/qml/places/items"
import "qrc:/qml/places/views"

CustomMapForm {
    id: page
    Plugin {
            id: plugin
            name: "osm" // "mapboxgl", "esri", ...
    }
    function setCircle(lon,lat,hAcc)
    {
        map.onCoordsReceived(lon,lat,hAcc)
    }
    SearchBar{
        id: searchBar
        width: parent.width
        anchors.top: parent.top
        height: 40
        onDoSearch: {
            if (searchText.length > 0)
                placeSearchModel.searchForText(searchText);
        }
    }
    MapComponent{
        id: map
        plugin: plugin
        anchors.top: searchBar.bottom
        height: parent.height - 40
        zoomLevel: (map.maximumZoomLevel - map.minimumZoomLevel)/2
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
    }
    Rectangle{
        id: searchRectangle
        anchors.fill: parent
        visible: false
        //! [PlaceSearchModel model]
        PlaceSearchModel {
            id: placeSearchModel
            plugin: plugin;
            searchArea: QtPositioning.circle(map.center, 10)
            favoritesPlugin: null
            relevanceHint: PlaceSearchModel.UnspecifiedHint
            function searchForCategory(category) {
                searchTerm = "";
                categories = category;
                recommendationId = "";
                searchArea = QtPositioning.circle(map.center, 10)
                limit = -1;
                update();
            }

            function searchForText(text) {
                searchTerm = text;
                categories = null;
                recommendationId = "";
                searchArea = QtPositioning.circle(map.center, 10)
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
                    console.log("ready")
                    if (count > 0)
                        console.log("visible!?")
                        searchRectangle.visible = true
                    break;
                case PlaceSearchModel.Error:
                    console.log("search place error: "+errorString())
                    //stackView.showMessage(qsTr("Search Place Error"),errorString())
                    break;
                }
            }
        }
        //! [PlaceSearchModel model]
        SearchResultView {
            id: searchView
        }
    }

    /*
    Rectangle{
        id: something
        anchors.fill: parent
        visible: false
    }*/
}
