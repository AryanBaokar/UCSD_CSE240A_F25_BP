#!/bin/bash
# Runs branch predictor commands and prints Misprediction Rate (transposed with 3 predictors)

traces=("U1_Blender" "U2_Leela" "U3_GCC" "U4_Cam4")
predictors=("tournament" "gshare" "custom")

output_file="predictor_results.txt"
> "$output_file"

# Print table header
header=$(printf "%-12s | %-12s | %-12s | %-12s\n" "Trace" "Tournament" "Gshare" "Custom")
divider=$(printf "%-12s-+-%-12s-+-%-12s-+-%-12s\n" "------------" "------------" "------------" "------------")
echo "$header" | tee -a "$output_file"
echo "$divider" | tee -a "$output_file"

# Loop through each trace and predictor
for trace in "${traces[@]}"; do
  
  # Run each predictor and extract misprediction rate
  t_rate=$(bunzip2 -kc "../traces/${trace}.bz2" | ./predictor --tournament 2>/dev/null \
            | grep -i "Misprediction rate" | awk -F':' '{print $2}' | xargs)
  [[ -z "$t_rate" ]] && t_rate="N/A"

  g_rate=$(bunzip2 -kc "../traces/${trace}.bz2" | ./predictor --gshare 2>/dev/null \
            | grep -i "Misprediction rate" | awk -F':' '{print $2}' | xargs)
  [[ -z "$g_rate" ]] && g_rate="N/A"

  c_rate=$(bunzip2 -kc "../traces/${trace}.bz2" | ./predictor --custom 2>/dev/null \
            | grep -i "Misprediction rate" | awk -F':' '{print $2}' | xargs)
  [[ -z "$c_rate" ]] && c_rate="N/A"

  # Print combined row
  printf "%-12s | %-12s | %-12s | %-12s\n" "$trace" "$t_rate" "$g_rate" "$c_rate" | tee -a "$output_file"
done

echo
echo "Results displayed above and saved to $output_file"
