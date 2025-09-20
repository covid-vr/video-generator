## Camera shots generator
On this subproject after the lung segmentation we generate a video of the lung with our trnsfer function applied.

> You need to execute the [covid-vr-env](https://github.com/covid-vr/covid-vr-docker) to prepare the environment.

Execute the following commands to take the lung picture views

```bash
vglrun {EXEC_PATH} -tf {TRANSFER_FUNCTION_PATH} -i {NII_PATH} -o {OUTPUT_PATH} -w {WIDTH} -h {HEIGHT} --c {BACKGROUND_COLOR_STR} -f {FRAMES} -t {TIME}
```

By default the values in our pipeline the values are:

- `BACKGROUND_COLOR_STR='255,255,255'`, white
- `TRANSFER_FUNCTION_PATH='/data/tf/tf6.xml'`, at that path we commit our best transfer function
- `OUTPUT_PATH='/data/videos'`
- `EXEC_PATH='/opt/video-generator/build/video-generator'`, generated in our [environment](https://github.com/covid-vr/covid-vr-docker)
- `WIDTH=512`, in pixels
- `HEIGHT=512`, in pixels
- `FRAMES=30`, frames per seconds
- `TIME=5`, time in seconds for the video

For test you can use in NII_PATH the 'data/P001.nii.gz'

We provided and example under /scripts/video-generator.sh
```bash
export DISPLAY=:0
vglrun ../build/video-generator -tf ../tf/tf6.xml -i ../data/P001.nii.gz -o ../data/P001.mp4 -w 512 -h 512 -c 255,255,255 -f 30 -t 5
```