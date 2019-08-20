import QtQuick 2.7
import QtLocation 5.7
import QtPositioning 5.7
/*
import QtQuick 2.12
import QtLocation 5.12
import QtPositioning 5.12
*/
Item{
    id: rootItem
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
    }
}
