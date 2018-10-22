# Antescofo Readme

This is a private fork of the original [Grame Guidolib Library](https://github.com/grame-cncm/guidolib/) from GitHub, for Antescofo's internal use.

Before any use, make note of the following facts:
- The main branch of this repository is `antescofo-master`. This branch is used to deploy to the _GuidoKit_ private Pod for iOS Projects.
- Other branches such as `master` and `dev` are used to track the original Grame project and update `antescofo-master` whenever appropriate.
- We use *Pull Requests* on `antescofo-master` branch

## Installation

- After cloning the project, open terminal and `cd` to the `build` folder. 
- Run the `BuildXCodeProject.sh` in terminal. This will create appropriate `iOS` and `MacOS` XCode projects and folders.

## Deploy to Pod (iOS)

Once the XCode Projects are ready, just run the `deploy2pod.sh` from the `build` folder. This will compile necessary objects, and copy appropriate libraries and sources to the _Pod Project Path_ given as argument.

## Contribution

Always branch out from `antescofo-master` and create pull-requests to that branch for Antescofo internal use.

## Upgrading the Library from Grame

Ideally you should have a secondary remote called `grame` pointing to `https://github.com/grame-cncm/libmusicxml.git`. The `dev` branch on that repository contains latest additions and upgrades.
To upgrade latest contributions from Grame, create a new Branch and merge those contributions with that of `antescofo-master` and contribute as usual by create a Pull Request.

---
# GRAME README : Welcome to the Guido project
======================================================================

[Grame](http://www.grame.fr) - Centre National de Création Musicale
----------------------------------------------------------------------

The Guido project is an open source project that encompasses a music notation format, a score rendering engine and various music score utilities. The Guido Notation Format is a general purpose formal language for representing score-level music in a platform-independent plain text and human readable way. The format comes with various software components for music score rendering and manipulation. The main of these components is the Guido Engine, a library embedable in various platforms and using different programming languages.

The Guido engine runs on the main operating systems: Linux, MacOS, iOS, Windows, Android.

See [Guido page](http://guidolib.sourceforge.net/) for more information.

See [Guido wiki](https://github.com/grame-cncm/guidolib/wiki) for building instructions.

Binary distributions are available from https://sourceforge.net/projects/guidolib/files/


---

Travis build status:  <a href="https://travis-ci.org/grame-cncm/guidolib"><img src="https://travis-ci.org/grame-cncm/guidolib.svg?branch=dev"></a>

----------------------------------------------------------------------

## Acknowledgments

The Guido project has been initiated in the 90s by Holger Hoos, Jürgen Kilian and Kai Renz, who designed the Guido Music Notation format and developped the core of the Guido engine. It became an open source project on 2002 at Grame initiative.

I would like to thank all the people who have been contributing to the project, and especially:

J. Scott Amort,
Jérôme Berthet,
Samuel Brochot,
Yannick Chapuis,
Thomas Coffy,
Christophe Daudin,
Colas Decron,
Guillaume Gouilloux,
Torben Hohn,
Camille Le Roy,
François Levy,
Arnaud Margaillan,
Benjamen Ruprechter,
Mike Solomon,
Ivan Vukosav

And of course, warmest thanks to my colleagues and friends Stéphane Letz
and Yann Orlarey.

---
[Dominique Fober](https://github.com/dfober)
