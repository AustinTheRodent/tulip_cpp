#!/bin/bash

script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
tulip_dsp=${script_dir%tulip_dsp_control*}"tulip_dsp_control"

echo $tulip_dsp/bin/main.exe -bypass

sudo $tulip_dsp/bin/main.exe -bypass
