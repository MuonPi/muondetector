import QtQuick 2.7
import QtLocation 5.7
import QtPositioning 5.7

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
    Map {
        id: map
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(50.569212, 8.673828) // 2. Physikalisches Institut Gießen
        zoomLevel: 14
    }
}
