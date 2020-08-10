from os import listdir

out = open('icons.cpp', 'w+')

out.writelines([
	'#define INCBIN_STYLE INCBIN_STYLE_SNAKE\n',
	'#include "third-party/incbin.h"\n',
	'#include "icons.h"\n\n'
])

count = 0

for category in listdir('third-party/icons'):

	for icon in listdir('third-party/icons/%s' % category):

		path = 'img/%s/%s' % (category, icon)
		name = '_'.join(icon.split('_')[1:-2])
		size = icon.split('_')[-1].strip('dp.png')
		reference = '%s.%s.%s' % (size, category, name)

		# INCBIN(ic_accessibility_black_18dp, "cgtb/third-party/icons/action/ic_accessibility_black_18dp.png");

		out.write('INCBIN(%s, "cgtb/third-party/icons/%s/%s");\n' % (icon.replace('.png', ''), category, icon))
		count = count + 1

out.write('\nconst std::map<std::string, std::pair<const unsigned char *, unsigned int>> cgtb::icons {\n')

for category in listdir('third-party/icons'):

	for icon in listdir('third-party/icons/%s' % category):

		path = 'img/%s/%s' % (category, icon)
		name = '_'.join(icon.split('_')[1:-2])
		size = icon.split('_')[-1].strip('dp.png')
		reference = '%s.%s.%s' % (size, category, name)

		# map : string -> pair, entry
		# { "18.toggle.star_border", { gic_ic_star_border_black_18dp_data, gic_star_border_black_18dp_size } },

		out.write('\t{ "%s", { g%s_data, g%s_size } }' % (reference, icon.replace('.png', ''), icon.replace('.png', '')))

		if count > 1: # don't put a comma after the last entry
			out.write(',')

		out.write('\n')

		count = count - 1

out.write('};\n')