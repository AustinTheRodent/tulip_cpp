#!/bin/bash

script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
tulip_dsp=${script_dir%overdrive_control*}"overdrive_control"

echo $tulip_dsp/bin/main.exe -bypass

sudo $tulip_dsp/bin/main.exe -bypass
