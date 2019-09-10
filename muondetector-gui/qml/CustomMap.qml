import QtQuick 2.2
import QtLocation 5.7
import QtQuick.Layouts 1.0
import QtPositioning 5.0
import QtQuick.Controls 2.1
import "qrc:/qml/places/items"
//import "qrc:/qml/places/views"

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
    /*
    SearchBar{
        id: searchBar
        width: parent.width
        anchors.top: parent.top
        height: 40
        onDoSearch: {
            if (searchText.length > 0)
                placeSearchModel.searchForText(searchText);
        }
    }*/
    MapComponent{
        id: map
        property double lastLon: 8.673828
        property double lastLat: 50.569212
        plugin: plugin
        anchors.top: parent.top
        //anchors.top: searchBar.bottom
        width: parent.width
        height: parent.height
        zoomLevel: (map.maximumZoomLevel - map.minimumZoomLevel)/2
        function onCoordsReceived(lon,lat,hAcc)
        {
            lastLon = lon
            lastLat = lat
            circle.center.longitude = lon
            circle.center.latitude = lat
            circle.radius = hAcc
/*
            console.log("lon: "+lon)
            console.log("lat: "+lat)
            console.log("lastLon: "+lastLon)
            console.log("lastLat: "+lastLat)
*/            if (control.checked){
                map.center = QtPositioning.coordinate(lat,lon)
            }
        }
        function jumpToLocation(){
            map.center = QtPositioning.coordinate(lastLat,lastLon)
        }

        MapCircle {
            id: circle
            center: parent.center
            radius: 0.0
            border.width: 2
        }
        ToolBar{
            id: buttonBar
            anchors.top: parent.top
            height: 30
            //Behavior on opacity { NumberAnimation{} }
            RowLayout{
                Rectangle{
                    height: 30
                    width: 60
                    anchors.top: parent.top
                    color: "white"
                    ToolButton{
                        id: centerButton
                        anchors.fill: parent
                        text: "center"
                        onClicked: map.jumpToLocation()
                    }
                }
                Rectangle{
                    Layout.column: 1
                    width: 90
                    anchors.top: parent.top
                    height: 30
                    color:"white"
                    CheckBox{
                        id: control
                        anchors.fill: parent
                        text: "follow"
                    }
                }
            }
        }
    }
    /*
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
    */
    /*
    Rectangle{
        id: something
        anchors.fill: parent
        visible: false
    }*/
}
