Get-ChildItem ./src -Include *.c,*.h -Recurs -File | Foreach { echo $_.fullname; clang-format -i $_.fullname }
Get-ChildItem ./inc -Include *.h -Recurs -File | Foreach { echo $_.fullname; clang-format -i $_.fullname }