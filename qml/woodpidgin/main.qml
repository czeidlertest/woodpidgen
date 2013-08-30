//import QtQuick 2.0
import QtQuick 1.0

Rectangle {
    width: 360
    height: 360

    function showRequestInfo(text)  {
        log.text = log.text + "\n" + text
        console.log(text)
    }

    Text  { id: log; anchors.fill: parent; anchors.margins: 10 }

    Rectangle {
        id: button
        anchors.horizontalCenter: parent.horizontalCenter; anchors.bottom: parent.bottom; anchors.margins: 10
        width: buttonText.width + 10; height: buttonText.height + 10
        border.width: mouseArea.pressed ? 2 : 1
        radius : 5; smooth: true

        Text  { id: buttonText; anchors.centerIn: parent; text: "Request php" }

        MouseArea  {
            id: mouseArea
            anchors.fill: parent

            onClicked:  {
                log.text = ""
                console.log("\n")
                var doc = new XMLHttpRequest();
                /*doc.onreadystatechange = function()  {

                    if (doc.readyState == XMLHttpRequest.HEADERS_RECEIVED)  {
                        showRequestInfo("Headers -->");
                        showRequestInfo(doc.getAllResponseHeaders());
                        showRequestInfo("Last modified -->");
                        showRequestInfo(doc.getResponseHeader("Last-Modified"));
                    } else if (doc.readyState == XMLHttpRequest.DONE)  {
                        showRequestInfo("Response Text -->");
                        showRequestInfo(doc.responseText);
                        showRequestInfo("XML Document Elements -->");
                        var a = doc.responseXML.documentElement;
                        for (var ii = 0; ii < a.childNodes.length; ++ii)  {
                            showRequestInfo(a.childNodes[ii].nodeName);
                        }
                        showRequestInfo("Headers -->");
                        showRequestInfo(doc.getAllResponseHeaders ());
                        showRequestInfo("Last modified -->");
                        showRequestInfo(doc.getResponseHeader("Last-Modified"));
                    }
                }
                doc.open("GET", "http://clemens-zeidler.de/RetrieveMessages.php");
                doc.send();
*/
                var url = "http://clemens-zeidler.de/RetrieveMessages.php";
                var params = "request=" + messageReceiver.getMessagesRequest();
                doc.open("POST", url, true);

                //Send the proper header information along with the request
                doc.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
                doc.setRequestHeader("Content-length", params.length);
                doc.setRequestHeader("Connection", "close");

                doc.onreadystatechange = function() {//Call a function when the state changes.
                    if(doc.readyState == 4 && doc.status == 200) {
                        //showRequestInfo(doc.responseText);
                    } else {
                        showRequestInfo(doc.responseText);
                        messageReceiver.messageDataReceived(doc.responseText, doc.responseText.length)
                    }
                }
                doc.send(params);
            }
        }
    }
}
