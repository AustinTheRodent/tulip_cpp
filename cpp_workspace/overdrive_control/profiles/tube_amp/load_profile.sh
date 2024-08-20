#!/bin/bash

script_dir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

settings_file=$script_dir/settings.ini

var_names=(fir_taps_file lut_entries_file symmetric_mode input_gain_lin output_gain_lin)
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

echo symmetric_mode: ${var_values[2]}

tulip_dsp=${script_dir%overdrive_control*}"overdrive_control"

if [ ${var_values[2]} == "true" ];then

  echo $tulip_dsp/bin/main.exe          \
    -fir $script_dir/${var_values[0]}   \
    -lut $script_dir/${var_values[1]}   \
    -gi_lin ${var_values[3]}            \
    -go_lin ${var_values[4]}            \

  sudo $tulip_dsp/bin/main.exe          \
    -fir $script_dir/${var_values[0]}   \
    -lut $script_dir/${var_values[1]}   \
    -gi_lin ${var_values[3]}            \
    -go_lin ${var_values[4]}            \

else

  echo $tulip_dsp/bin/main.exe          \
    -fir $script_dir/${var_values[0]}   \
    -lut $script_dir/${var_values[1]}   \
    -gi_lin ${var_values[3]}            \
    -go_lin ${var_values[4]}            \
    -nonsymmetric                       \

  sudo $tulip_dsp/bin/main.exe          \
    -fir $script_dir/${var_values[0]}   \
    -lut $script_dir/${var_values[1]}   \
    -gi_lin ${var_values[3]}            \
    -go_lin ${var_values[4]}            \
    -nonsymmetric                       \

fi
