The following howto note describes the generation of a *release version* for the
Microblaze software:

1) push your development branch to github

2) merge the branch into **master** on github with a *pull request*

3) now, on your local computer, pull the updated master branch

4) the revision number gets updated in constant_defs.h on the *master* branch with
the following convention:
```
<3>.<year_of_release>.<day_of_the_year>
```

This version number can be generated at the bash prompt as follows:

```% date +3.%g.%-j```

e.g. 3.21.36

5) make the necessary changes to the constant_defs.h:
```
#define EMBEDDED_SOFTWARE_VERSION_MAJOR   3

#define EMBEDDED_SOFTWARE_VERSION_MINOR   21     /* year */

#define EMBEDDED_SOFTWARE_VERSION_PATCH   36     /* day of the year */
```

6) Now commit the changes to your local master branch. The first line of the commit
should be 'Release v3.<year>.<day_of_year>'
e.g. Release v3.21.36

7) you can then note the changes in the body of the commit text

8) after committing, tag the commit with release version number e.g.'git tag v3.21.36'

9) then push the commit: 'git push --tags origin master'

10) run ```make``` to generate elf
