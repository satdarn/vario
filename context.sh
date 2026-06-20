#!/usr/bin/env bash
rm context.md
touch context.md 
ls -r >> context.md
for file in ./interpreter/*; do
	[ -f "$file" ] || continue
	printf "\n## %s\n \`\`\` " "$file" >> context.md
	cat "$file" >> context.md 
	printf "\n \`\`\`\n" >> context.md
done
echo "# Makefile" >> context.md 
echo "\`\`\` Make" >> context.md
cat Makefile >> context.md 
echo "\`\`\`" >> context.md
make 2>> context.md 
echo "Done"
