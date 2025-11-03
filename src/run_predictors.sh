#!/bin/bash
# Runs branch predictor commands and formats output into a table

# Define traces and predictors
traces=("U1_Blender" "U2_Leela" "U3_GCC" "U4_Cam4")
predictors=("tournament" "gshare")

# Temporary file for output
output_file="predictor_results.txt"
> "$output_file"  # clear file

# Print table header
printf "%-12s | %-12s | %-60s\n" "Trace" "Predictor" "Result Summary" | tee -a "$output_file"
printf "%-12s-+-%-12s-+-%-60s\n" "------------" "------------" "------------------------------------------------------------" | tee -a "$output_file"

# Run commands
for trace in "${traces[@]}"; do
  for predictor in "${predictors[@]}"; do
    echo "Running $trace with $predictor..."
    result=$(bunzip2 -kc ../traces/${trace}.bz2 | ./predictor --${predictor} 2>/dev/null | tr -d '\r' | tr '\n' ' ')
    printf "%-12s | %-12s | %-60s\n" "$trace" "$predictor" "$result" | tee -a "$output_file"
  done
done

echo
echo "Results saved to $output_file"
