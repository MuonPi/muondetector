import QtQuick 2.2
import QtLocation 5.7
import QtQuick.Layouts 1.0
import QtPositioning 5.0
import QtQuick.Controls 2.0
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
        map.onCoordsReceived(lon,lat,0,hAcc,0,hAcc)
    }
    function setCoordinates(lon,lat,alt,hor_err,vert_err,tot_err)
    {
        map.onCoordsReceived(lon,lat,alt,hor_err,vert_err,tot_err)
    }
    function setEnabled(enabled)
    {
        map.onEnable(enabled)
    }
    signal coordinateSignal(double lat, double lon)
    MapComponent{
        id: map
        property double lastLon: 8.673828
        property double lastLat: 0.569212
        property double lastAlt: 0.
        plugin: plugin
        anchors.top: parent.top
        width: parent.width
        height: parent.height
        zoomLevel: (map.maximumZoomLevel - map.minimumZoomLevel)/2
        function onCoordsReceived(lon,lat,alt,hor_err,vert_err,tot_err)
        {
            lastLon = lon
            lastLat = lat
            lastAlt = alt
            circle.center = QtPositioning.coordinate(lat,lon)
            marker.center = circle.center
            marker.radius=1*(1<<map.maximumZoomLevel)/(1<<map.zoomLevel)
            circle.radius = tot_err
            if (control.checked){
                map.center = circle.center
            }
            var lon_str = "lon: %1"
            var lat_str = "lat: %1"
            var alt_str = "alt: %1"
            var pos_err_str = "err: %1 m"
            lonLabel.text=lon_str.arg(lon)
            latLabel.text=lat_str.arg(lat)
            altLabel.text=alt_str.arg(alt)
            posErrLabel.text=pos_err_str.arg(tot_err)
        }
        function jumpToLocation(){
            map.center = QtPositioning.coordinate(lastLat,lastLon)
        }
        MouseArea {
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            anchors.fill: parent
            onPressed: {
                if (mouse.button === Qt.RightButton) { // 'mouse' is a MouseEvent argument passed into the onClicked signal handler
                    var coordinates = map.toCoordinate(Qt.point(mouse.x,mouse.y));
                    circle.center = coordinates;
                    marker.center = coordinates;
                    page.coordinateSignal(coordinates.latitude, coordinates.longitude);
                }
            }
        }
        function onEnable(enabled) {
            if (enabled === false) {
                circle.radius = 0
                marker.radius = 0
                lonLabel.text="lon: N/A"
                latLabel.text="lat: N/A"
                altLabel.text="alt: N/A"
                posErrLabel.text="err: N/A"
            }
        }
        MapCircle {
            id: circle
            center: parent.center
            radius: 0.0
            border.width: 1
            border.color: "blue"
            color: Qt.rgba(0.0, 0.0, 0.5, 0.25)
        }
        MapCircle {
            id: marker
            center: parent.center
            radius: 0.0
            border.width: 1
            border.color: "black"
            color: Qt.rgba(1.0, 0.0, 0.5, 0.8)
        }
        ColumnLayout{
            anchors.top: parent.top
            Frame{
                id: buttonBar
                //anchors.top: parent.top
                height: 30
                spacing: 0
                Layout.row: 0
                RowLayout{
                    Rectangle{
                        Layout.column: 0
                        height: 30
                        width: 60
                        Layout.alignment: Qt.AlignTop
                        color: "white"
                        ToolButton{
                            id: centerButton
                            anchors.fill: parent
                            text: "center"
                            onClicked: map.jumpToLocation()
                            background: Rectangle {
                                    color: parent.down ? "#bbbbbb" :
                                            (parent.hovered ? "#d6d6d6" : "#f6f6f6")
                            }
                        }
                    }
                    Rectangle{
                        Layout.column: 1
                        width: 90
                        Layout.alignment: Qt.AlignTop
                        height: 30
                        color:"white"
                        CheckBox{
                            id: control
                            anchors.fill: parent
                            text: "follow"
                            checked: true
                            background: Rectangle {
                                    color: parent.down ? "#bbbbbb" :
                                            (parent.hovered ? "#d6d6d6" : "#f6f6f6")
                            }
                        }
                    }
                }
            }
            Frame{
                Layout.row: 1
                width: 200
                Layout.alignment: Qt.AlignTop
                height: 50
                ColumnLayout{
                    Label {
                        Layout.row: 0
                        id: lonLabel
                        text: "lon"
                    }
                    Label {
                        Layout.row: 1
                        id: latLabel
                        text: "lat"
                    }
                    Label {
                        Layout.row: 2
                        id: altLabel
                        text: "alt"
                    }
                    Label {
                        Layout.row: 3
                        id: posErrLabel
                        text: "err"
                    }
                }
            }
        }
    }
}
