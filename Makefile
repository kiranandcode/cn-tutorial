.PHONY: default clean exercises 

SRC_EXAMPLES=$(wildcard src/examples/*)
SOLUTIONS=$(patsubst src/examples/%, build/solutions/%, $(SRC_EXAMPLES))
EXERCISES=$(patsubst src/examples/%, build/exercises/%, $(SRC_EXAMPLES))

default: build exercises build/tutorial.html 

clean:
	rm -rf build

build:
	mkdir -p build
	mkdir -p build/exercises
	mkdir -p build/solutions

##############################################################################
# Exercises

exercises: $(EXERCISES) $(SOLUTIONS)

build/exercises/%: src/examples/%
#	sed -E '/^--BEGIN--$$/,/^--END--$$/d' $< > $@
	sed -E '\|^.*--BEGIN--.*$$|,\|^.*--END--.*$$|d' $< > $@

build/solutions/%: src/examples/%
#	cat $< | sed '/^--BEGIN--$$/d' | sed '/^--END--$$/d' > $@
	cat $< | sed '\|^.*--BEGIN--.*$$|d' | sed '\|^.*--END--.*$$|d' > $@
	if [[ "$<" != *"broken"* ]]; then echo cn $@ && cn $@; fi

##############################################################################
# Tutorial document

build/tutorial.adoc: src/tutorial.adoc
	cp $< $@

build/images: src/images
	cp -r $< $@

build/tutorial.html: build/tutorial.adoc $(SRC_EXAMPLES) build/images
	asciidoctor --doctype book $< -o $@

##############################################################################
# Site-specific stuff

upenn-install: default
	rm -rf $(HOME)/pub/courses/6700-SL-2024/current/CN
	mkdir $(HOME)/pub/courses/6700-SL-2024/current/CN
	cp -r build/tutorial.html build/images $(HOME)/pub/courses/6700-SL-2024/current/CN

testsed: 
	sed -E '\|^.*--BEGIN--.*$$|,\|^.*--END--.*$$|d' test.tmp 
	cat test.tmp | sed '\|^.*--BEGIN--.*$$|d' | sed '\|^.*--END--.*$$|d' 
