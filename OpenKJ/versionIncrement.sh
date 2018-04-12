#!/bin/bash
oldmaj=`grep MAJOR okjversion.h| cut -d " " -f 3`
oldmin=`grep MINOR okjversion.h| cut -d " " -f 3`
oldbld=`grep BUILD okjversion.h| cut -d " " -f 3`
branch=`grep BRANCH okjversion.h| cut -d " " -f 3`
newbld=$(($oldbld+1))

rm -f okjversion.h

echo "#ifndef OKJVERSION_H" > okjversion.h
echo "#define OKJVERSION_H" >> okjversion.h
echo >> okjversion.h
echo "#define OKJ_VERSION_MAJOR $oldmaj" >> okjversion.h
echo "#define OKJ_VERSION_MINOR $oldmin" >> okjversion.h
echo "#define OKJ_VERSION_BUILD $newbld" >> okjversion.h
echo "#define OKJ_VERSION_STRING \"$oldmaj.$oldmin.$newbld\"" >> okjversion.h
echo "#define OKJ_VERSION_BRANCH $branch" >> okjversion.h
echo >> okjversion.h
echo "#endif //OKJVERSION_H" >> okjversion.h

