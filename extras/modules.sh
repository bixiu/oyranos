#!/bin/bash

function usage() {
  echo "Usage: $0 [-f|-h]"
  echo
  echo -e "-f\tPrint full outpout (slow)"
  echo -e "-h\tShow this help message"
  echo
  echo Legend:
  echo "[CDC] == Constructor / Destructor / Copy constructor"
  echo "Green color: git log says it's been imported"
  echo "Red color: git log unable to find any relative message"

  exit 0
}

case $1 in
  -h) usage ;;
  -f) FUNC=1 ;;
  -b) FUNC=2 ;;
  *)  FUNC=0 ;;
esac

function grp_color() {
  case $1 in
    cmm_handling)       COLOR=1 ;;
    module_api)         COLOR=2 ;;
    objects_conversion) COLOR=3 ;;
    objects_generic)    COLOR=4 ;;
    objects_value)      COLOR=5 ;;
    objects_profile)    COLOR=6 ;;
    objects_image)      COLOR=7 ;;
  esac
}

function ctor() {
  PATTERN="Implement the constructor for oy${1}_s"
  git log --oneline | grep -q "$PATTERN" && c=2 || c=1
  tput setaf $c
  echo -n C
  tput sgr0
}

function dtor() {
  PATTERN="Implement the destructor for oy${1}_s"
  git log --oneline | grep -q "$PATTERN" && c=2 || c=1
  tput setaf $c
  echo -n D
  tput sgr0
}

function cctor() {
  PATTERN="Implement the copy constructor for oy${1}_s"
  git log --oneline | grep -q "$PATTERN" && c=2 || c=1
  tput setaf $c
  echo -n C
  tput sgr0
}

function privateAPI() {
  PATTERN="Import private methods for oy${1}_s"
  git log --oneline | grep -q "$PATTERN" && c=2 || c=1
  tput setaf $c
  echo -n Private
  tput sgr0
}

function publicAPI() {
  PATTERN="Import public methods for oy${1}_s"
  git log --oneline | grep -q "$PATTERN" && c=2 || c=1
  tput setaf $c
  echo -n Public
  tput sgr0
}

function builtPublic() {
  PUBLIC="API_generated/CMakeFiles/oyranos_object.dir/oy$1_s.c.o"
  test -f $PUBLIC && c=2 || c=1
  tput setaf $c
  echo -n Public
  tput sgr0
}

function builtPrivate() {
  PRIVATE="API_generated/CMakeFiles/oyranos_object.dir/oy$1_s_.c.o"
  test -f $PRIVATE && c=2 || c=1
  tput setaf $c
  echo -n Private
  tput sgr0
}

function print_full_list() {
  for d in sources/*dox sources_weg/*dox; do
    GROUP=$(grep '@ingroup' $d | awk '{print $3}')
    CLASS=$(basename $d .dox)
    test $CLASS = "Class" && continue
    grp_color $GROUP
    tput setaf $COLOR
    echo -n $GROUP
    tput sgr0
    echo -en ": $CLASS\t\t\t["
    ctor $CLASS
    dtor $CLASS
    cctor $CLASS
    echo -n ']'
    echo -n ', Imported API: ['
    publicAPI $CLASS
    echo -n ':'
    privateAPI $CLASS
    echo ']'
  done
}

function print_list() {
  for d in sources/*dox sources_weg/*dox; do
    GROUP=$(grep '@ingroup' $d | awk '{print $3}')
    CLASS=$(basename $d .dox)
    test $CLASS = "Class" && continue
    grp_color $GROUP
    tput setaf $COLOR
    echo -n $GROUP
    tput sgr0
    echo ": $CLASS"
  done
}

function print_build_list() {
  for d in sources/*dox sources_weg/*dox; do
    GROUP=$(grep '@ingroup' $d | awk '{print $3}')
    CLASS=$(basename $d .dox)
    test $CLASS = "Class" && continue
    grp_color $GROUP
    tput setaf $COLOR
    echo -n $GROUP
    tput sgr0
    echo -en ": $CLASS\t\t\t["
    builtPublic $CLASS
    echo -n '|'
    builtPrivate $CLASS
    echo ']'
  done
}

case $FUNC in
  '0') print_list | sort ;;
  '1') print_full_list | sort ;;
  '2') print_build_list | sort ;;
esac

exit 0
