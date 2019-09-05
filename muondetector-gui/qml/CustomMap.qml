import QtQuick 2.4
import QtLocation 5.7
import QtQuick.Layouts 1.0
import QtPositioning 5.0
import "qrc:/qml/places/items"
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
    //ColumnLayout{
        SearchBar{
            id: searchBar
            width: parent.width
            anchors.top: parent.top
            height: 50
        }
        MapComponent{
            id: map
            plugin: plugin
            anchors.top: searchBar.bottom
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
 //   }
}
