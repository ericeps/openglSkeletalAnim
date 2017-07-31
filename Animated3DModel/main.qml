import QtQuick 2.0
import QtQuick.Controls 1.0

Rectangle {
    width: 600
    height: 100
    color: "beige"

    Row {
        height: parent.height
        spacing: 20
        Rectangle {
            height: parent.height
            width: rotateButton.width + rotateButtons.width
            Button {
                id: rotateButton
                property var __listModel: [
                    { "text": "Rotate X" },
                    { "text": "Rotate Y" },
                    { "text": "Rotate Z" },
                ]
                property int __curIndex: 0

                text: __listModel[__curIndex].text
                onClicked: {
                    __curIndex = __curIndex+1 > 2 ? 0 : __curIndex+1
                }
            }
            Row {
                id: rotateButtons
                anchors.top: rotateButton.bottom
                spacing: 20
                Button {
                    width: 20
                    text: "-"
                }
                Button {
                    width: 20
                    text: "+"
                }
            }
        }
        Rectangle {
            height: parent.height
            width: scaleText.width + scaleButtons.width
            Text {
                id: scaleText
                text: qsTr("Scale")
            }
            Row {
                id: scaleButtons
                anchors.top: scaleText.bottom
                spacing: 20
                Button {
                    width: 20
                    text: "-"
                }
                Button {
                    width: 20
                    text: "+"
                }
            }
        }
        Rectangle {
            height: parent.height
            width: translateButton.width + translateButtons.width
            Button {
                id: translateButton
                property var __listModel: [
                    { "text": "Translate X" },
                    { "text": "Translate Y" },
                    { "text": "Translate Z" },
                ]
                property int __curIndex: 0

                text: __listModel[__curIndex].text
                onClicked: {
                    __curIndex = __curIndex+1 > 2 ? 0 : __curIndex+1
                }
            }
            Row {
                id: translateButtons
                anchors.top: translateButton.bottom
                spacing: 20
                Button {
                    width: 20
                    text: "-"
                    onClicked: {
                        if (translateButton.__curIndex === 0)
                            sceneController.translate(.1, 0, 0)
                        else if (translateButton.__curIndex === 1)
                            sceneController.translate(0, .1, 0)
                        else if (translateButton.__curIndex === 2)
                            sceneController.translate(0, 0, .1)
                    }
                }
                Button {
                    width: 20
                    text: "+"
                    onClicked: {
                        if (translateButton.__curIndex === 0)
                            sceneController.translate(-.1, 0, 0)
                        else if (translateButton.__curIndex === 1)
                            sceneController.translate(0, -.1, 0)
                        else if (translateButton.__curIndex === 2)
                            sceneController.translate(0, 0, -.1)
                    }
                }
            }
        }
    }
}
