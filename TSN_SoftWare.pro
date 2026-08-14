TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS += native_sender native_receiver gui

native_sender.file = native/sender_ordered/sender_ordered.pro
native_receiver.file = native/receiver/receiver.pro
gui.file = gui/gui.pro
