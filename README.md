# cellScan
NSA cell scaner

CellScan is a free and open-source project for NSA cell coverage check.

CellScan is released under the AGPLv3 license and uses software from the srsLTE project. 

Replace the same file in srsLTE project. Make and  run the  Pdsch_ue in srsLte/lib/examples.

"Pdsch_ue -f 1850000000 -u 100 -U 127.0.0.1 " will get  with -f frequency cell sib msg in wireshark MAC_LTE format. UpperLayerIndication-r15 in sib2 indicates this is a lte anchor cell for NSA.

测试位置和 @老师好我叫何同学 在北邮的测试地点一样。同一区域，移动同band的LTE锚点小区最小驻留电平要比联通的高8db，在覆盖上，可见移动还是投入更大一些。