#!/bin/sh

# rubibtex, based on the original version contained in the t2 bundle.
# Thomas Esser, Public Domain.

progname=rubibtex
tmpdir=${TMPDIR-${TEMP-${TMP-/tmp}}}/$progname.$$
job=$1
backup="$tmpdir/orig.aux"

case $job in
  "")
    echo "usage: $progname file" >&2
    exit 1
esac

if test ! -f "$job.aux"; then
  echo "$progname: file \`$job.aux' does not exist." >&2
  exit 1
fi

trap '
  rm -rf "$tmpdir"
  exit 1
' 1 2 3 7 13 15
(umask 077; mkdir "$tmpdir") \
  || { echo "$progname: could not create directory \`$tmpdir'" >&2; exit 1; }

cat <"$job.aux" >"$backup" || {
  echo "$progname: could not create backup of file \`$job.aux' as \`$backup'." >&2
  rm -rf "$tmpdir"
  exit 1
}

sed '
  /^\\citation/ {
    s/\\IeC {\\CYRA }/�/g
    s/\\IeC {\\CYRB }/�/g
    s/\\IeC {\\CYRV }/�/g
    s/\\IeC {\\CYRG }/�/g
    s/\\IeC {\\CYRD }/�/g
    s/\\IeC {\\CYRE }/�/g
    s/\\IeC {\\CYRYO }/�/g
    s/\\IeC {\\CYRZH }/�/g
    s/\\IeC {\\CYRZ }/�/g
    s/\\IeC {\\CYRI }/�/g
    s/\\IeC {\\CYRISHRT }/�/g
    s/\\IeC {\\CYRK }/�/g
    s/\\IeC {\\CYRL }/�/g
    s/\\IeC {\\CYRM }/�/g
    s/\\IeC {\\CYRN }/�/g
    s/\\IeC {\\CYRO }/�/g
    s/\\IeC {\\CYRP }/�/g
    s/\\IeC {\\CYRR }/�/g
    s/\\IeC {\\CYRS }/�/g
    s/\\IeC {\\CYRT }/�/g
    s/\\IeC {\\CYRU }/�/g
    s/\\IeC {\\CYRF }/�/g
    s/\\IeC {\\CYRH }/�/g
    s/\\IeC {\\CYRC }/�/g
    s/\\IeC {\\CYRCH }/�/g
    s/\\IeC {\\CYRSH }/�/g
    s/\\IeC {\\CYRSHCH }/�/g
    s/\\IeC {\\CYRHRDSN }/�/g
    s/\\IeC {\\CYRERY }/�/g
    s/\\IeC {\\CYRSFTSN }/�/g
    s/\\IeC {\\CYREREV }/�/g
    s/\\IeC {\\CYRYU }/�/g
    s/\\IeC {\\CYRYA }/�/g
    s/\\IeC {\\cyra }/�/g
    s/\\IeC {\\cyrb }/�/g
    s/\\IeC {\\cyrv }/�/g
    s/\\IeC {\\cyrg }/�/g
    s/\\IeC {\\cyrd }/�/g
    s/\\IeC {\\cyre }/�/g
    s/\\IeC {\\cyryo }/�/g
    s/\\IeC {\\cyrzh }/�/g
    s/\\IeC {\\cyrz }/�/g
    s/\\IeC {\\cyri }/�/g
    s/\\IeC {\\cyrishrt }/�/g
    s/\\IeC {\\cyrk }/�/g
    s/\\IeC {\\cyrl }/�/g
    s/\\IeC {\\cyrm }/�/g
    s/\\IeC {\\cyrn }/�/g
    s/\\IeC {\\cyro }/�/g
    s/\\IeC {\\cyrp }/�/g
    s/\\IeC {\\cyrr }/�/g
    s/\\IeC {\\cyrs }/�/g
    s/\\IeC {\\cyrt }/�/g
    s/\\IeC {\\cyru }/�/g
    s/\\IeC {\\cyrf }/�/g
    s/\\IeC {\\cyrh }/�/g
    s/\\IeC {\\cyrc }/�/g
    s/\\IeC {\\cyrch }/�/g
    s/\\IeC {\\cyrsh }/�/g
    s/\\IeC {\\cyrshch }/�/g
    s/\\IeC {\\cyrhrdsn }/�/g
    s/\\IeC {\\cyrery }/�/g
    s/\\IeC {\\cyrsftsn }/�/g
    s/\\IeC {\\cyrerev }/�/g
    s/\\IeC {\\cyryu }/�/g
    s/\\IeC {\\cyrya }/�/g
  }
' <"$backup" >"$job.aux"

bibtex "$job"

cat "$backup" > "$job.aux"
rm -rf "$tmpdir"
exit 0
