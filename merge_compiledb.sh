#!/bin/bash
# Generate compile databases for both environments and merge them
cd "$(dirname "$0")" || exit 1

echo "Generating compile_commands.json for sender..."
pio run -e sender -t compiledb > /dev/null 2>&1

# Save sender's compile_commands.json
cp compile_commands.json compile_commands_sender.json

echo "Generating compile_commands.json for receiver..."
pio run -e receiver -t compiledb > /dev/null 2>&1

# Save receiver's compile_commands.json
cp compile_commands.json compile_commands_receiver.json

echo "Merging compile databases..."
python3 << 'EOF'
import json

# Load both databases
with open('compile_commands_sender.json', 'r') as f:
    sender_db = json.load(f)

with open('compile_commands_receiver.json', 'r') as f:
    receiver_db = json.load(f)

# Merge (receiver entries may overwrite sender, but they both compile the same files)
merged = sender_db + receiver_db

# Remove duplicates by file path
seen = {}
unique = []
for entry in merged:
    file_path = entry['file']
    if file_path not in seen:
        seen[file_path] = True
        unique.append(entry)

# Write merged database
with open('compile_commands.json', 'w') as f:
    json.dump(unique, f, indent=2)

print(f"Merged {len(unique)} unique entries")
EOF

echo "compile_commands.json is ready for both environments."
