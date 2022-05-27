export DISPLAY=:0
vglrun ../build/video-generator -tf ../tf/tf6.xml -i ../data/P001.nii.gz -o ../data/P001.mp4 -w 512 -h 512 -c 255,255,255 -f 30 -t 5
