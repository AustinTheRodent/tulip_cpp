#!/bin/bash

echo 0 > /home/ubuntu/startup_scripts/data/current_output_volume_db.txt
sudo /home/ubuntu/cpp_workspace/wm8960_codec_setup/bin/main.exe
echo ""
#/home/ubuntu/cpp_workspace/tulip_dsp_control/profiles/transparent/load_profile.sh
sudo /home/ubuntu/cpp_workspace/tulip_dsp_init/bin/main.exe
