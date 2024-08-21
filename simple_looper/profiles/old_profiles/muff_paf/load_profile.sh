#!/bin/bash

script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

settings_file=$script_dir/settings.ini

var_names=(fir_taps_file poly0_taps_file poly1_taps_file input_gain_lin output_gain_lin)
var_values=$var_names

input=$settings_file
while IFS= read -r line
do

  for i in $(seq 1 ${#var_names[@]});
  do
    var_name=${var_names[i-1]}
    if [[ $line = *$var_name* ]]; then
      #echo ${line:${#var_name}+1:${#line}-${#var_name}}
      var_values[i-1]=${line:${#var_name}+1:${#line}-${#var_name}}
    fi
  done

done < $input

for i in $(seq 1 ${#var_names[@]});
do
  echo ${var_names[i-1]} = ${var_values[i-1]}
done

tulip_dsp=${script_dir%tulip_dsp_control*}"tulip_dsp_control"
echo $tulip_dsp/bin/main.exe          \
  -fir $script_dir/${var_values[0]}   \
  -poly0 $script_dir/${var_values[1]} \
  -poly1 $script_dir/${var_values[2]} \
  -gi_lin ${var_values[3]}            \
  -go_lin ${var_values[4]}            \

sudo $tulip_dsp/bin/main.exe          \
  -fir $script_dir/${var_values[0]}   \
  -poly0 $script_dir/${var_values[1]} \
  -poly1 $script_dir/${var_values[2]} \
  -gi_lin ${var_values[3]}            \
  -go_lin ${var_values[4]}            \
