/*
 * Copyright (c) 2011 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include "xconverter.h"

static char *html_to_entry(AppData *ad, int type_index, const char *str);
static char *efl_to_entry(AppData *ad, int type_index, const char *str);
static char *text_to_entry(AppData *ad, int type_index, const char *str);
static char *image_path_to_entry(AppData *ad, int type_index, const char *str);
static char *polaris_to_entry(AppData *ad, int type_index, const char *str);

//static char *make_close_tag(Eina_List* nodes);
static char *do_not_convert(AppData *ad, int type_index, const char *str);
static char *html_to_efl(AppData *ad, int type_index, const char *str);
static char *efl_to_html(AppData *ad, int type_index, const char *str);
static char *text_to_html(AppData *ad, int type_index, const char *str);
static char *text_to_efl(AppData *ad, int type_index, const char *str);
static char *to_text(AppData *ad, int type_index, const char *str);
static char *image_path_to_html(AppData *ad, int type_index, const char *str);
static char *image_path_to_efl(AppData *ad, int type_index, const char *str);
//static char *image_path_to_text(AppData *ad, int type_index, const char *str);
//static char *efl_to_efl(AppData *ad, int type_index, const char *str);
//static char *html_to_html(AppData *ad, int type_index, const char *str);
static char *image_path_to_image_path(AppData *ad, int type_index, const char *str);
static char *html_to_image_path(AppData *ad, int type_index, const char *str);
static char *efl_to_image_path(AppData *ad, int type_index, const char *str);

ENTRY_EMOTICON_S emotion_name_table[ENTRY_EMOTICON_MAX] = {
	[ENTRY_EMOTICON_HAPPY] = {"emoticon/happy", ":-)"},
	[ENTRY_EMOTICON_SORRY] = {"emoticon/sorry", ":-("},
	[ENTRY_EMOTICON_WINK] = {"emoticon/wink", ";-)"},
	[ENTRY_EMOTICON_TONGUE_DANGLING] = {"emoticon/tongue-dangling", ":-P"},
	[ENTRY_EMOTICON_SUPRISED] = {"emoticon/surprised", "=-O"},
	[ENTRY_EMOTICON_KISS] = {"emoticon/kiss", ":-*"},
	[ENTRY_EMOTICON_ANGRY_SHOUT] = {"emoticon/angry-shout", ":O"},
	[ENTRY_EMOTICON_SMILE] = {"emoticon/smile", "B-)"},
	[ENTRY_EMOTICON_OMG] = {"emoticon/omg", ":-["},
	[ENTRY_EMOTICON_LITTLE_BIT_SORRY] = {"emoticon/little-bit-sorry", ":-\\"},
	[ENTRY_EMOTICON_VERY_SORRY] = {"emoticon/very-sorry", ":'("},
	[ENTRY_EMOTICON_GUILTY] = {"emoticon/guilty", ":-X"},
	[ENTRY_EMOTICON_HAHA] = {"emoticon/haha", ":-D"},
	[ENTRY_EMOTICON_WORRIED] = {"emoticon/worried", "o_O"},
	[ENTRY_EMOTICON_LOVE] = {"emoticon/love", "&lt;3"}, //<3
	[ENTRY_EMOTICON_EVIL] = {"emoticon/evil", "x-("},
	[ENTRY_EMOTICON_HALF_SMILE] = {"emoticon/half-smile", ":-/"},
	[ENTRY_EMOTICON_MINIMAL_SMILE] = {"emoticon/minimal-smile", ":-|"},
};

int atom_type_index_get(AppData *ad, Ecore_X_Atom atom)
{
	int i, j;
	for (i = 0; i < ATOM_INDEX_MAX; i++)
	{
		for (j = 0; j < ad->targetAtoms[i].atom_cnt; j++)
			if (ad->targetAtoms[i].atom[j] == atom)
				return i;
	}
	return -1;
}

void init_target_atoms(AppData *ad)
{
	int atom_cnt[ATOM_INDEX_MAX] = {
		1, 5, 2, 1, 2, 1
	};
	char *targetAtomNames[][5] = {
		{ "TARGETS" },
		{ "UTF8_STRING", "STRING", "TEXT", "text/plain;charset=utf-8", "text/plain" },
		{ "text/html;charset=utf-8", "text/html" },
		{ "application/x-elementary-markup" },
		{ "text/uri", "text/uri-list" },
		{ "polaris-markup" }
	};
	text_converter_func converts_to_entry[ATOM_INDEX_MAX] = {
		NULL, text_to_entry, html_to_entry, efl_to_entry, image_path_to_entry, polaris_to_entry
	};

	text_converter_func converts[ATOM_INDEX_MAX][ATOM_INDEX_MAX] = {
		{NULL, NULL, NULL, NULL, NULL, NULL},
		{NULL, do_not_convert, text_to_html, text_to_efl, NULL, NULL},
		{NULL, to_text, do_not_convert, html_to_efl, html_to_image_path, NULL},
		{NULL, to_text, efl_to_html, do_not_convert, do_not_convert, NULL},
		{NULL, NULL, image_path_to_html, image_path_to_efl, image_path_to_image_path, NULL},
		{NULL, to_text, NULL, NULL, NULL, do_not_convert}
	};

	int i, j;
	for (i = 0; i < ATOM_INDEX_MAX; i++)
	{
		ad->targetAtoms[i].atom_cnt = atom_cnt[i];
		ad->targetAtoms[i].name = MALLOC(sizeof(char *) * atom_cnt[i]);
		ad->targetAtoms[i].atom = MALLOC(sizeof(Ecore_X_Atom) * atom_cnt[i]);
		for (j = 0; j < atom_cnt[i]; j++)
		{
			DBG("atomName: %s", targetAtomNames[i][j]);
			ad->targetAtoms[i].name[j] = SAFE_STRDUP(targetAtomNames[i][j]);
			ad->targetAtoms[i].atom[j] = ecore_x_atom_get(targetAtomNames[i][j]);
		}
		ad->targetAtoms[i].convert_to_entry = converts_to_entry[i];

		for (j = 0; j < ATOM_INDEX_MAX; j++)
			ad->targetAtoms[i].convert_to_target[j] = converts[i][j];
		//ecore_x_selection_converter_atom_add(ad->targetAtoms[i].atom, target_converters[i]);
		//ecore_x_selection_converter_atom_add(ad->targetAtoms[i].atom, generic_converter);
	}
}

void depose_target_atoms(AppData *ad)
{
	int i, j;
	for (i = 0; i < ATOM_INDEX_MAX; i++)
	{
		for (j = 0; j < ad->targetAtoms[i].atom_cnt; j++)
		{
			if (ad->targetAtoms[i].name[j])
				FREE(ad->targetAtoms[i].name[j]);
		}
		if (ad->targetAtoms[i].name)
			FREE(ad->targetAtoms[i].name);
		if (ad->targetAtoms[i].atom)
			FREE(ad->targetAtoms[i].atom);
	}
}

static Eina_Bool targets_converter(AppData *ad, Ecore_X_Atom reqAtom, CNP_ITEM *item, void **data_ret, int *size_ret, Ecore_X_Atom *ttype, int *tsize)
{
	CALLED();

	int count;
	int i, j;
	int item_type_index = ATOM_INDEX_TEXT;
	char *file = NULL;

	if (item)
	{
		item_type_index = item->type_index;
		file = item->file;
	}

	if (!file || item->img_from_web || item->img_from_markup)
	{
		if (item_type_index == ATOM_INDEX_HTML)
			ad->targetAtoms[item_type_index].convert_to_target[ATOM_INDEX_IMAGE] = NULL;
		else if (item_type_index == ATOM_INDEX_EFL)
			ad->targetAtoms[item_type_index].convert_to_target[ATOM_INDEX_IMAGE] = NULL;
	}

	for (i = 0, count = 0; i < ATOM_INDEX_MAX; i++)
	{
		if (ad->targetAtoms[item_type_index].convert_to_target[i])
			count += ad->targetAtoms[i].atom_cnt;
	}

	*data_ret = MALLOC(sizeof(Ecore_X_Atom) * count);
	DBG("item_type: %d, target Atom cnt: %d", item_type_index, count);
	if (!*data_ret)
		return EINA_FALSE;

	for (i = 0, count = 0; i < ATOM_INDEX_MAX; i++)
	{
		if (ad->targetAtoms[item_type_index].convert_to_target[i])
		{
			for(j = 0; j < ad->targetAtoms[i].atom_cnt; j++)
			{
				((Ecore_X_Atom *)*data_ret)[count++] = ad->targetAtoms[i].atom[j];
				DBG("send target atom: %s", ad->targetAtoms[i].name[j]);
			}
		}
	}

	if (!file)
	{
		if (item_type_index == ATOM_INDEX_HTML)
			ad->targetAtoms[item_type_index].convert_to_target[ATOM_INDEX_IMAGE] = html_to_image_path;
		else if (item_type_index == ATOM_INDEX_EFL)
			ad->targetAtoms[item_type_index].convert_to_target[ATOM_INDEX_IMAGE] = efl_to_image_path;
	}

	if (size_ret) *size_ret = count;
	if (ttype) *ttype = ECORE_X_ATOM_ATOM;
	if (tsize) *tsize = 32;
	return EINA_TRUE;
}

Eina_Bool generic_converter(AppData *ad, Ecore_X_Atom reqAtom, CNP_ITEM *item, void **data_ret, int *size_ret, Ecore_X_Atom *ttype, int *tsize)
{
	CALLED();

	if (ad->targetAtoms[ATOM_INDEX_TARGET].atom[0] == reqAtom)
		return targets_converter(ad, reqAtom, item, data_ret, size_ret, ttype, tsize);

	int req_index = atom_type_index_get(ad, reqAtom);
	int item_type_index = ATOM_INDEX_TEXT;
	void *item_data = "";

	if (req_index < 0) return EINA_FALSE;

	if (item)
	{
		item_type_index = item->type_index;
		item_data = item->data;
	}

	if (ad->targetAtoms[item_type_index].convert_to_target[req_index])
	{
		*data_ret = ad->targetAtoms[item_type_index].convert_to_target[req_index](ad, item_type_index, item_data);
		if (!*data_ret)
			return EINA_FALSE;
		if (size_ret) *size_ret = SAFE_STRLEN(*data_ret);
		if (ttype) *ttype = ad->targetAtoms[item_type_index].atom[0];
		if (tsize) *tsize = 8;
		return EINA_TRUE;
	}

	return EINA_FALSE;
}

/* For convert EFL to HTML */

#define TAGPOS_START    0x00000001
#define TAGPOS_END      0x00000002
#define TAGPOS_ALONE    0x00000003

/* TEXTBLOCK tag using stack but close tag word has no mean maybe bug...
 * TEXTBLOCK <b>bold<font>font</b>bold</font>
 * HTML <b>bold<font>font bold</b>font</font> */

typedef struct _TagTable {
	char *src;
	char *dst;
}TagTable;

TagTable _EFLtoHTMLConvertTable[] = {
	{"font", "font"},
	{"underline", "del"},
	{"strikethrough", "ins"},
	{"br", "br"},
	{"br/", "br"},
	{"ps", "br"},
	{"b", "b"},
	{"item", "img"}
};

TagTable _HTMLtoEFLConvertTable[] = {
	{"font", ""},
	{"del", "underline"},
	{"u", "underline"},
	{"ins", "strikethrough"},
	{"s", "strikethrough"},
	{"br", "br"},
	{"b", "b"},
	{"strong", "b"},
	{"img", "item"}
};


typedef struct _TagNode TagNode, *PTagNode;
struct _TagNode {
	char *tag;  //EINA_STRINGSHARE if NULL just str
	char *tag_str;
	char *str;
	const char *pos_in_ori_str;
	PTagNode matchTag;
	void *tagData;
	unsigned char tagPosType;
};

typedef struct _FontTagData FontTagData, *PFontTagData;
struct _FontTagData {
	char *name;
	char *color;
	char *size;
	char *bg_color;
};


typedef struct _ItemTagData ItemTagData, *PItemTagData;
struct _ItemTagData {
	char *href;
	char *width;
	char *height;
};

#define SAFEFREE(ptr) \
	do\
{\
	if (ptr)\
	FREE(ptr);\
	ptr = NULL;\
} while(0);\

#define freeAndAssign(dst, value) \
	do\
{\
	if (value)\
	{\
		SAFEFREE(dst);\
		dst = value;\
	}\
} while(0);

static PTagNode _new_tag_node(char *tag, char *tag_str, char* str, const char *pos_in_ori_str);
static PTagNode _get_start_node(const char *str);
static PTagNode _get_next_node(PTagNode prev);
static void _delete_node(PTagNode node);
static void _link_match_tags(Eina_List *nodes);
static char *_get_tag_value(const char *tag_str, const char *tag_name);
static char *_convert_to_html(Eina_List* nodes);
static void _set_EFL_tag_data(Eina_List* nodes);
static char *_convert_to_edje(Eina_List* nodes);
static void _set_HTML_tag_data(Eina_List* nodes);
static void cleanup_tag_list(Eina_List *nodeList);
static PFontTagData _set_EFL_font_data(PFontTagData data, const char *tag_str);
static PItemTagData _set_EFL_item_data(PItemTagData data, const char *tag_str);
static PFontTagData _set_HTML_font_data(PFontTagData data, const char *tag_str);
static PItemTagData _set_HTML_img_data(PItemTagData data, const char *tag_str);

static void _dumpNode(Eina_List* nodes);

static PTagNode
_new_tag_node(char *tag, char *tag_str, char* str, const char *pos_in_ori_str)
{
	PTagNode newNode = CALLOC(1, sizeof(TagNode));
	if (tag)
		eina_str_tolower(&tag);
	newNode->tag = tag;
	newNode->tag_str = tag_str;
	newNode->str = str;
	newNode->pos_in_ori_str = pos_in_ori_str;
	return newNode;
}

static PTagNode
_get_start_node(const char *str)
{
	char *startStr = NULL;
	if (!str || str[0] == '\0')
		return NULL;

	if (str[0] != '<')
	{
		char *tagStart = SAFE_STRCHR(str, '<');
		if (!tagStart)
			startStr = SAFE_STRDUP(str);
		else
		{
			int strLength = tagStart - str;
			startStr = MALLOC(sizeof(char) * (strLength + 1));
			SAFE_STRNCPY(startStr, str, strLength);
			startStr[strLength] = '\0';
		}
	}

	return _new_tag_node(NULL, NULL, startStr, str);
}

static PTagNode
_get_next_node(PTagNode prev)
{
	PTagNode retTag = NULL;
	char *tagStart;
	char *tagEnd;
	char *tagNameEnd = NULL;
	char *nextTagStart;

	if (prev->tag == NULL)
		tagStart = SAFE_STRCHR(prev->pos_in_ori_str, '<');
	else
		tagStart = SAFE_STRCHR(prev->pos_in_ori_str + 1, '<');

	if (!tagStart)
		return retTag;

	tagEnd = SAFE_STRCHR(tagStart, '>');
	nextTagStart = SAFE_STRCHR(tagStart + 1, '<');

	if (!tagEnd || (nextTagStart && (nextTagStart < tagEnd)))
		return _get_start_node(tagStart + 1);

	int spCnt = 5;
	char *spArray[spCnt];
	spArray[0] = SAFE_STRCHR(tagStart, '=');
	spArray[1] = SAFE_STRCHR(tagStart, '_');
	spArray[2] = SAFE_STRCHR(tagStart, ' ');
	spArray[3] = SAFE_STRCHR(tagStart, '\t');
	spArray[4] = SAFE_STRCHR(tagStart, '\n');
	tagNameEnd = tagEnd;

	int i;
	for (i = 0; i < spCnt; i++)
	{
		if (spArray[i] && spArray[i] < tagNameEnd)
			tagNameEnd = spArray[i];
	}

	int tagLength = tagNameEnd - tagStart - 1;
	char *tagName = NULL;
	if (!SAFE_STRNCMP(&tagStart[1], "/item", tagLength))
		tagName = SAFE_STRDUP("");
	else
		tagName = SAFE_STRNDUP(&tagStart[1], tagLength);

	int tagStrLength = 0;
	char *tagStr = NULL;
	if (tagName)
	{
		tagStrLength = tagEnd - tagStart + 1;
		tagStr = SAFE_STRNDUP(tagStart, tagStrLength);
	}

	unsigned int strLength = nextTagStart ? (unsigned int)(nextTagStart - tagEnd - 1) : SAFE_STRLEN(&tagEnd[1]);
	char *str = SAFE_STRNDUP(&tagEnd[1], strLength);

	retTag = _new_tag_node(tagName, tagStr, str, tagStart);
	return retTag;
}


static void
_delete_node(PTagNode node)
{
	if (node)
	{
		SAFEFREE(node->tag_str);
		SAFEFREE(node->str);

		if (node->tagData)
		{
			if (node->tag)
			{
				if (!SAFE_STRCMP("font", node->tag))
				{
					PFontTagData data = node->tagData;
					SAFEFREE(data->name);
					SAFEFREE(data->color);
					SAFEFREE(data->size);
					SAFEFREE(data->bg_color);
				}
				if (!SAFE_STRCMP("item", node->tag))
				{
					PItemTagData data = node->tagData;
					SAFEFREE(data->href);
					SAFEFREE(data->width);
					SAFEFREE(data->height);
				}
			}
			SAFEFREE(node->tagData);
		}
		SAFEFREE(node->tag);
		SAFEFREE(node);
	}
}

static void
_link_match_tags(Eina_List *nodes)
{
	Eina_List *stack = NULL;

	PTagNode trail, popData;
	Eina_List *l, *r;

	EINA_LIST_FOREACH(nodes, l, trail)
	{
		if (!trail || !trail->tag || trail->tag[0] == '\0') continue;

		if (!SAFE_STRCMP("br", trail->tag) || !SAFE_STRCMP("br/", trail->tag))
		{
			trail->tagPosType = TAGPOS_ALONE;
			continue;
		}
		else if (!SAFE_STRCMP("item", trail->tag) || !SAFE_STRCMP("img", trail->tag))
		{
			trail->tagPosType = TAGPOS_ALONE;
			continue;
		}

		if (trail->tag[0] != '/') // PUSH
		{
			stack = eina_list_append(stack, trail);
			/*             eina_array_push(stack, trail);
						   DBG("stack: %d, tag %s", eina_array_count_get(stack), trail->tag);*/
			DBG("stack: %d, tag %s", eina_list_count(stack), trail->tag);
		}
		else // POP
		{
			if (!eina_list_count(stack))
			{
				WRN("tag not matched %s", trail->tag);
				continue;
			}

			EINA_LIST_REVERSE_FOREACH(stack, r, popData)
			{
				if (!popData || !popData->tag || popData->tag[0] == '\0') continue;

				if (!SAFE_STRCMP(popData->tag, &trail->tag[1]))
				{
					popData->tagPosType = TAGPOS_START;
					trail->tagPosType = TAGPOS_END;
					popData->matchTag = trail;
					trail->matchTag = popData;
					stack = eina_list_remove_list(stack, r);
					break;
				}
			}
			/*             popData = eina_array_pop(stack);

						   popData->tagPosType = TAGPOS_START;
						   trail->tagPosType = TAGPOS_END;
						   popData->matchTag = trail;
						   trail->matchTag = popData;
						   DBG("pop stack: %d, tag %s", eina_array_count_get(stack), trail->tag);
			 */
		}
	}

	/*   if (eina_array_count_get(stack))
		 DBG("stack state: %d, tag %s", eina_array_count_get(stack), trail->tag);*/

	/* Make Dummy close tag */
	/*   while ((popData = eina_array_pop(stack)))  */

	EINA_LIST_REVERSE_FOREACH(stack, r, popData)
	{
		if (!popData) continue;

		PTagNode newData;
		int tagLength = SAFE_STRLEN(popData->tag);
		char *tagName = MALLOC(sizeof(char) * (tagLength + 2));
		if (!tagName) break;

		tagName[0] = '/';
		tagName[1] = '\0';
		SAFE_STRCAT(tagName, popData->tag);

		newData = _new_tag_node(tagName, NULL, NULL, NULL);
		popData->tagPosType = TAGPOS_START;
		newData->tagPosType = TAGPOS_END;
		popData->matchTag = newData;
		newData->matchTag = popData;
		nodes = eina_list_append(nodes, newData);
		/*        DBG("stack: %d, tag %s", eina_array_count_get(stack), popData->tag);*/
	}
	/*   DBG("stack_top: %d", eina_array_count_get(stack));
		 eina_array_free(stack);*/
	eina_list_free(stack);
}

static char *
_get_tag_value(const char *tag_str, const char *tag_name)
{
	if (!tag_name || !tag_str)
		return NULL;

	char *tag;
	if ((tag = SAFE_STRSTR(tag_str, tag_name)))
	{
		if (tag[SAFE_STRLEN(tag_name)] == '_')
			return NULL;
		char *value = SAFE_STRCHR(tag, '=');
		if (value)
		{
			do
			{
				value++;
			} while (!isalnum(*value) && *value != '#');

			int spCnt = 6;
			char *spArray[spCnt];
			spArray[0] = SAFE_STRCHR(value, ' ');
			spArray[1] = SAFE_STRCHR(value, '>');
			spArray[2] = SAFE_STRCHR(value, '\"');
			spArray[3] = SAFE_STRCHR(value, '\'');
			spArray[4] = SAFE_STRCHR(value, '\t');
			spArray[5] = SAFE_STRCHR(value, '\n');
			char *valueEnd = SAFE_STRCHR(value, '\0');

			int i;
			int start = 0;
			if ((!SAFE_STRNCMP(tag_str, "<item", 5) && !SAFE_STRCMP(tag_name, "href")) // EFL img tag
               || (!SAFE_STRNCMP(tag_str, "<img", 4) && !SAFE_STRCMP(tag_name, "src"))) // HTML img tag
               start = 1;

			for (i = start; i < spCnt; i++)
			{
				if (spArray[i] && spArray[i] < valueEnd)
					valueEnd = spArray[i];
			}

			int valueLength = valueEnd - value;
			return SAFE_STRNDUP(value, valueLength);
		}
	}
	return NULL;
}

static PFontTagData
_set_EFL_font_data(PFontTagData data, const char *tag_str)
{
	char *value;

	if (!data)
		data = CALLOC(1, sizeof(FontTagData));
	value = _get_tag_value(tag_str, "font_size");
	freeAndAssign(data->size, value);
	value = _get_tag_value(tag_str, "color");
	freeAndAssign(data->color, value);
	value = _get_tag_value(tag_str, "bgcolor");
	freeAndAssign(data->bg_color, value);
	value = _get_tag_value(tag_str, "font");
	freeAndAssign(data->name, value);

	return data;
}

static PItemTagData
_set_EFL_item_data(PItemTagData data, const char *tag_str)
{
	char *value;

	if (!data)
		data = CALLOC(1, sizeof(ItemTagData));
	value = _get_tag_value(tag_str, "href");
	if (value)
	{
		char *path = SAFE_STRSTR(value, "file://");
		if (path)
		{
			char *modify = MALLOC(sizeof(char) * (SAFE_STRLEN(value) + 1));
			SAFE_STRNCPY(modify, "file://", 8);
			path += 7;
			while (path[1] && path[0] && path[1] == '/' && path[0] == '/')
			{
				path++;
			}
			SAFE_STRCAT(modify, path);
			data->href = modify;
			DBG("image href ---%s---", data->href);
			FREE(value);
		}
		else
			freeAndAssign(data->href, value);
	}

	value = _get_tag_value(tag_str, "absize");
	if (value)
	{
		char *xpos = SAFE_STRCHR(value, 'x');
		if (xpos)
		{
			int absizeLen = SAFE_STRLEN(value);
			freeAndAssign(data->width, SAFE_STRNDUP(value, xpos - value));
			freeAndAssign(data->height, SAFE_STRNDUP(xpos + 1, absizeLen - (xpos - value) - 1));
			DBG("image width: -%s-, height: -%s-", data->width, data->height);
		}
		FREE(value);
	}
	return data;
}

static void
_set_EFL_tag_data(Eina_List* nodes)
{
	PTagNode trail;
	Eina_List *l;

	EINA_LIST_FOREACH(nodes, l, trail)
	{
		if (!trail || !trail->tag || trail->tag[0] == '\0') continue;

		if (!SAFE_STRCMP("font", trail->tag) || !SAFE_STRCMP("color", trail->tag))
			trail->tagData = _set_EFL_font_data(trail->tagData, trail->tag_str);
		else if (!SAFE_STRCMP("item", trail->tag))
			trail->tagData = _set_EFL_item_data(trail->tagData, trail->tag_str);
	}
}

static PFontTagData
_set_HTML_font_data(PFontTagData data, const char *tag_str)
{
	char *value;

	if (!data)
		data = CALLOC(1, sizeof(FontTagData));
	value = _get_tag_value(tag_str, "size");
	freeAndAssign(data->size, value);
	value = _get_tag_value(tag_str, "color");
	freeAndAssign(data->color, value);
	value = _get_tag_value(tag_str, "bgcolor");
	freeAndAssign(data->bg_color, value);
	value = _get_tag_value(tag_str, "face");
	freeAndAssign(data->name, value);

	return data;
}

static PItemTagData
_set_HTML_img_data(PItemTagData data, const char *tag_str)
{
	char *value;

	if (!data)
		data = CALLOC(1, sizeof(ItemTagData));
	value = _get_tag_value(tag_str, "src");
	if (value)
	{
		char *path = SAFE_STRSTR(value, "file://");
		if (path)
		{
			char *modify = MALLOC(sizeof(char) * (SAFE_STRLEN(value) + 1));
			SAFE_STRNCPY(modify, "file://", 8);
			path += 7;
			while (path[1] && path[0] && path[1] == '/' && path[0] == '/')
			{
				path++;
			}
			SAFE_STRCAT(modify, path);
			data->href = modify;
			DBG("image src ---%s---", data->href);
			FREE(value);
		}
		else
			freeAndAssign(data->href, value);
	}

	value = _get_tag_value(tag_str, "width");
	freeAndAssign(data->width, value);
	value = _get_tag_value(tag_str, "height");
	freeAndAssign(data->height, value);
	return data;
}

static void
_set_HTML_tag_data(Eina_List* nodes)
{
	PTagNode trail;
	Eina_List *l;

	EINA_LIST_FOREACH(nodes, l, trail)
	{
		if (!trail || !trail->tag || trail->tag[0] == '\0') continue;

		if (!SAFE_STRCMP("font", trail->tag) || !SAFE_STRCMP("color", trail->tag))
			trail->tagData = _set_HTML_font_data(trail->tagData, trail->tag_str);
		else if (!SAFE_STRCMP("img", trail->tag))
			trail->tagData = _set_HTML_img_data(trail->tagData, trail->tag_str);
	}
}

static void
_dumpNode(Eina_List* nodes)
{
	PTagNode trail;
	Eina_List *l;

	EINA_LIST_FOREACH(nodes, l, trail)
	{
		if (!trail) continue;

		DBG("tag: %s, tag_str: %s, str: %s, tagPosType: %d",
				trail->tag, trail->tag_str, trail->str, trail->tagPosType);
		DBG("matchTag: %x", (unsigned int)trail->matchTag);
		if (trail->matchTag)
			DBG("matchTag->tag_str: %s", trail->matchTag->tag_str);
		if (trail->tagData)
		{
			if (!SAFE_STRCMP(trail->tag, "font"))
			{
				PFontTagData data = trail->tagData;
				DBG(" tagData->name: %s, tagData->color: %s, tagData->size: %s, tagData->bg_color: %s",
						data->name, data->color, data->size, data->bg_color);
			}
			else if (!SAFE_STRCMP(trail->tag, "item") || !SAFE_STRCMP(trail->tag, "img"))
			{
				PItemTagData data = trail->tagData;
				DBG(" tagData->href: %s, tagData->width: %s, tagData->height: %s",
						data->href, data->width, data->height);
			}
			else
				WRN("ERROR!!!! not need tagData");
		}
	}
}

static char *
_convert_to_html(Eina_List* nodes)
{
	PTagNode trail;
	Eina_List *l;

	Eina_Strbuf *html = eina_strbuf_new();

	int tableCnt = sizeof(_EFLtoHTMLConvertTable) / sizeof(TagTable);

	EINA_LIST_FOREACH(nodes, l, trail)
	{
		if (!trail) continue;

		if (trail->tag)
		{
			char *tagName = trail->tagPosType == TAGPOS_END ?
				trail->matchTag->tag : trail->tag;
			int j;
			for(j = 0; j < tableCnt; j++)
			{
				if (!SAFE_STRCMP(_EFLtoHTMLConvertTable[j].src, tagName))
				{
					switch(trail->tagPosType)
					{
						case TAGPOS_END:
							eina_strbuf_append(html, "</");
							break;
						default:
							eina_strbuf_append(html, "<");
							break;
					}

					eina_strbuf_append(html, _EFLtoHTMLConvertTable[j].dst);
					if (trail->tagPosType != TAGPOS_END)
					{
						if (!SAFE_STRCMP(_EFLtoHTMLConvertTable[j].src, "font"))
						{
							PFontTagData data = trail->tagData;
							if (data->name)
							{
							}
							if (data->color)
							{
								char *color = SAFE_STRDUP(data->color);
								if (color && color[0] == '#' && SAFE_STRLEN(color) == 9)
								{
									color[7] = '\0';
									eina_strbuf_append_printf(html, " color=\"%s\"", color);
								}
								else
									eina_strbuf_append_printf(html, " color=\"%s\"", data->color);
								if (color)
									FREE(color);
							}
							if (data->size)
								eina_strbuf_append_printf(html, " size=\"%s\"", data->size);
							if (data->bg_color)
							{
							}
						}
						else if (!SAFE_STRCMP(_EFLtoHTMLConvertTable[j].src, "item"))
						{
							PItemTagData data = trail->tagData;
							if (data->href)
								eina_strbuf_append_printf(html, " src=\"%s\"", data->href);
							if (data->width)
								eina_strbuf_append_printf(html, " width=\"%s\"", data->width);
							if (data->height)
								eina_strbuf_append_printf(html, " height=\"%s\"", data->height);
						}
					}
					switch(trail->tagPosType)
					{
						/* closed tag does not need in HTML
						   case TAGPOS_ALONE:
						   eina_strbuf_append(html, " />");
						   break;*/
						default:
							eina_strbuf_append(html, ">");
							break;
					}
					break;
				}
			}
		}
		if (trail->str)
			eina_strbuf_append(html, trail->str);
	}

	eina_strbuf_replace_all(html, "  ", " &nbsp;");
	char *ret = eina_strbuf_string_steal(html);
	eina_strbuf_free(html);
	return ret;
}

#define IMAGE_DEFAULT_WIDTH "240"
#define IMAGE_DEFAULT_HEIGHT "180"


static char *
_convert_to_edje(Eina_List* nodes)
{
	PTagNode trail;
	Eina_List *l;

	Eina_Strbuf *edje = eina_strbuf_new();

	int tableCnt = sizeof(_HTMLtoEFLConvertTable) / sizeof(TagTable);

	EINA_LIST_FOREACH(nodes, l, trail)
	{
		if (!trail) continue;

		if (trail->tag)
		{
			char *tagName = trail->tagPosType == TAGPOS_END ?
				trail->matchTag->tag : trail->tag;
			int j;
			for(j = 0; j < tableCnt; j++)
			{
				if (!SAFE_STRCMP(_HTMLtoEFLConvertTable[j].src, tagName))
				{
					if (_HTMLtoEFLConvertTable[j].dst[0] != '\0')
					{
						switch(trail->tagPosType)
						{
							case TAGPOS_END:
								eina_strbuf_append(edje, "</");
								break;
							default:
								eina_strbuf_append(edje, "<");
								break;
						}

						eina_strbuf_append(edje, _HTMLtoEFLConvertTable[j].dst);
					}
					if (trail->tagPosType != TAGPOS_END)
					{
						if (!SAFE_STRCMP(_HTMLtoEFLConvertTable[j].src, "font"))
						{
							PFontTagData data = trail->tagData;
							if (data->name)
							{
							}
							if (data->color)
							{
								if (data->color[0] == '#' && SAFE_STRLEN(data->color) == 7)
									eina_strbuf_append_printf(edje, "<color=%sff>", data->color);
								else
									eina_strbuf_append_printf(edje, "<color=%s>", data->color);

							}
							if (data->size)
								eina_strbuf_append_printf(edje, "<font_size=%s>", data->size);
							if (data->bg_color)
							{
							}
							break;
						}
						else if (!SAFE_STRCMP(_HTMLtoEFLConvertTable[j].src, "img"))
						{
							PItemTagData data = trail->tagData;
							char *width = IMAGE_DEFAULT_WIDTH, *height = IMAGE_DEFAULT_HEIGHT;
							if (data->width)
								width = data->width;
							if (data->height)
								height = data->height;
							eina_strbuf_append_printf(edje, " absize=%sx%s", width, height);
							if (data->href)
								eina_strbuf_append_printf(edje, " href=%s></item>", data->href);
							break;
						}
					}
					else
					{
						if (_HTMLtoEFLConvertTable[j].dst[0] == '\0')
						{
							if (!SAFE_STRCMP(_HTMLtoEFLConvertTable[j].src, "font"))
							{
								if (trail->matchTag->tagData)
								{
									PFontTagData data = trail->matchTag->tagData;
									if (data->name)
									{
									}
									if (data->color)
										eina_strbuf_append_printf(edje, "</color>");
									if (data->size)
										eina_strbuf_append_printf(edje, "</font>");
									if (data->bg_color)
									{
									}
									break;
								}
							}
						}
					}
					switch(trail->tagPosType)
					{
						/* not support in efl
						   case TAGPOS_ALONE:
						   eina_strbuf_append(edje, " />");
						   break;
						 */
						default:
							eina_strbuf_append(edje, ">");
							break;
					}
					break;
				}
			}/* for(j = 0; j < tableCnt; j++) end */
		}
		if (trail->str)
			eina_strbuf_append(edje, trail->str);
	}

	eina_strbuf_replace_all(edje, "&nbsp;", " ");
	char *ret = eina_strbuf_string_steal(edje);
	eina_strbuf_free(edje);
	return ret;
}

char *string_for_entry_get(AppData *ad, int type_index, const char *str)
{
	DBG("type_index: %d str: %s ", type_index, str);
	if (ad->targetAtoms[type_index].convert_to_entry)
		return ad->targetAtoms[type_index].convert_to_entry(ad, type_index, str);
	return NULL;
}

char *string_for_image_path_get(AppData *ad, int type_index, const char *str)
{
	DBG("type_index: %d str: %s ", type_index, str);
	char *image_path = NULL;

	if (type_index == ATOM_INDEX_HTML)
		image_path = html_to_image_path(ad, type_index, str);
	else if (type_index == ATOM_INDEX_EFL)
		image_path = efl_to_image_path(ad, type_index, str);

	return image_path;
}
/*
static char *make_close_tag(Eina_List* nodes)
{
	CALLED();
	PTagNode trail;
	Eina_List *l;

	Eina_Strbuf *tag_str = eina_strbuf_new();

	EINA_LIST_FOREACH(nodes, l, trail)
	{
		if (trail->tag)
		{
			if (trail->tag_str)
				eina_strbuf_append(tag_str, trail->tag_str);
			else
			{
				eina_strbuf_append(tag_str, "<");
				eina_strbuf_append(tag_str, trail->tag);
				eina_strbuf_append(tag_str, ">");
			}
		}
		if (trail->str)
			eina_strbuf_append(tag_str, trail->str);
	}

	char *ret = eina_strbuf_string_steal(tag_str);
	eina_strbuf_free(tag_str);
	return ret;
}
*/
static char *do_not_convert(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	if (type_index != ATOM_INDEX_HTML)
	{
		char *emoticon_text = entry_convert_emoticon_to_normal_text(str);
		return emoticon_text;
	}
	return SAFE_STRDUP(str);
}
/*
   static char *efl_to_efl(AppData *ad, int type_index, const char *str)
   {
   CALLED();
   return NULL;
   }

   static char *html_to_html(AppData *ad, int type_index, const char *str)
   {
   CALLED();
   return NULL;
   }
 */

#define IMAGE_DEFAULT_WIDTH "240"
#define IMAGE_DEFAULT_HEIGHT "180"
static char *make_image_path_tag(int type_index, const char *str)
{
	char *img_tag_str = "file://%s";
	char *efl_img_tag = "<item absize="IMAGE_DEFAULT_WIDTH"x"IMAGE_DEFAULT_HEIGHT" href=file://%s>";
	char *html_img_tag = "<img src=\"file://%s\">";

	switch (type_index)
	{
		case ATOM_INDEX_HTML:
			img_tag_str = html_img_tag;
			break;
		case ATOM_INDEX_EFL:
			img_tag_str = efl_img_tag;
			break;
		case ATOM_INDEX_TEXT:
		case ATOM_INDEX_IMAGE:
			break;
		default:
			ERR("wrong type_index: %d", type_index);
			return NULL;
	}

	size_t len = snprintf(NULL, 0, img_tag_str, str) + 1;
	char *ret = MALLOC(sizeof(char) * len);
	if (ret)
		snprintf(ret, len, img_tag_str, str);
	return ret;
}

/*
static char *image_path_to_text(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	return make_image_path_tag(ATOM_INDEX_TEXT, str);
}
*/

static char *image_path_to_html(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	return make_image_path_tag(ATOM_INDEX_HTML, str);
}

static char *image_path_to_efl(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	return make_image_path_tag(ATOM_INDEX_EFL, str);
}

static char *image_path_to_image_path(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	return make_image_path_tag(ATOM_INDEX_IMAGE, str);
}

static char *html_to_image_path(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	Eina_Strbuf *sbuf = eina_strbuf_new();
	Eina_Bool image_path_exists = EINA_FALSE;
	int len = SAFE_STRLEN(str);
	char *p = (char *)str;
	char *s;
	char *image_path = NULL;

	if (type_index == ATOM_INDEX_HTML)
	{
		for (s = p; (p - s) <= len; p++)
		{
			if (*p == '<')
			{
				if (!SAFE_STRNCMP((p + 1), "img", 3))
				{
					for (p += 4; *p != '"'; p++);
					if (!SAFE_STRNCMP((p + 1), "http:/", 6) || !SAFE_STRNCMP((p + 1), "file:/", 6))
					{
						if (!SAFE_STRNCMP((p + 1), "http:/", 6))
							ad->clipdrawer->http_path = EINA_TRUE;
						else
							ad->clipdrawer->http_path = EINA_FALSE;

						p += 7;
						s = p;
						for (; *p != '"'; p++);
						eina_strbuf_append_length(sbuf, s, p - s);
						image_path_exists = EINA_TRUE;
						break;
					}
					else if ((*(p + 1) == '/'))
					{
						p++;
						s = p;
						for (; *p != '"'; p++);
						eina_strbuf_append_length(sbuf, s, p - s);
						image_path_exists = EINA_TRUE;
						ad->clipdrawer->http_path = EINA_FALSE;
						break;
					}
				}
			}
		}
	}

	if (image_path_exists)
	{
		//Replace the space Unicode character(%20).
		eina_strbuf_replace_all(sbuf, "%20", " ");
		image_path = eina_strbuf_string_steal(sbuf);
		eina_strbuf_free(sbuf);
		return image_path;
	}

	eina_strbuf_free(sbuf);
	return NULL;
}

static char *efl_to_image_path(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	Eina_Strbuf *sbuf = eina_strbuf_new();
	Eina_Bool image_path_exists = EINA_FALSE;
	int len = SAFE_STRLEN(str);
	char *p = entry_convert_emoticon_to_normal_text((char *)str);
	char *s, *temp;
	char *image_path = NULL;
	temp = p;
	if (type_index == ATOM_INDEX_EFL)
	{
		for (s = p; (p - s) <= len; p++)
		{
			if (*p == '<')
			{
				if (!SAFE_STRNCMP((p + 1), "item", 3))
				{
					for (p += 5; *p != 'h'; p++);
					if (!SAFE_STRNCMP(p, "href=file:/", 11))
					{
						p += 11;
						s = p;
						for (; *p != '>'; p++);
						eina_strbuf_append_length(sbuf, s, p - s);
						image_path_exists = EINA_TRUE;
						break;
					}
					else if (!SAFE_STRNCMP(p, "href=", 5))
					{
						p += 5;
						s = p;
						for (; *p != '>'; p++);
						eina_strbuf_append_length(sbuf, s, p - s);
						image_path_exists = EINA_TRUE;
						break;
					}
				}
			}
		}
	}
	FREE(temp);
	if (image_path_exists)
	{
		image_path = eina_strbuf_string_steal(sbuf);
		eina_strbuf_free(sbuf);
		return image_path;
	}

	eina_strbuf_free(sbuf);
	return NULL;
}

static char *markup_to_entry(AppData *ad, int type_index, const char *str)
{
	CALLED();
	if (!str)
		return NULL;

	Eina_Strbuf *strbuf = eina_strbuf_new();
	if (!strbuf)
		return SAFE_STRDUP(str);
	eina_strbuf_prepend(strbuf, "<font_size=28><color=#000000FF>");

	const char *trail = str;

	while (trail && *trail)
	{
		const char *pretrail = trail;
		unsigned long length;
		char *temp;
		char *endtag;

		trail = SAFE_STRCHR(trail, '<');
		if (!trail)
		{
			eina_strbuf_append(strbuf, pretrail);
			break;
		}
		endtag = SAFE_STRCHR(trail, '>');
		if (!endtag)
			break;

		length = trail - pretrail;

		temp = SAFE_STRNDUP(pretrail, length);
		if (!temp)
		{
			trail++;
			continue;
		}

		eina_strbuf_append(strbuf, temp);
		FREE(temp);
		trail++;

		if (trail[0] == '/')
		{
			trail = endtag + 1;
			continue;
		}

		if (!SAFE_STRNCMP(trail, "br", 2))
		{
			eina_strbuf_append(strbuf, "<br>");
			trail = endtag + 1;
			continue;
		}
		trail = endtag + 1;
	}

	if (type_index == ATOM_INDEX_HTML)
		eina_strbuf_replace_all(strbuf, "&nbsp;", " ");

	char *entry_str = eina_strbuf_string_steal(strbuf);
	eina_strbuf_free(strbuf);
	return entry_str;
}

static char *polaris_to_entry(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	return markup_to_entry(ad, type_index, str);
}

static char *html_to_entry(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	return markup_to_entry(ad, type_index, str);
}

static int entry_emoticon_origin_string(char **src_text, int* item_length)
{
	int i = 0;
	char *start_item = NULL;
	char *end_item = NULL;
	char *emoticon = NULL;

	int start_item_pos = 0;
	int end_item_pos = 0;
	int emoticon_pos = 0;

	char href_buf[50] = {0, };

	start_item = strstr(*src_text, "<item");

	if (start_item)
		start_item_pos = strlen(*src_text) - strlen(start_item);

	end_item = strchr(*src_text, '>');

	if (end_item)
		end_item_pos = strlen(*src_text) - strlen(end_item);

	if (start_item_pos >= end_item_pos)
		return ENTRY_EMOTICON_NONE;

	for (i = 1; i < ENTRY_EMOTICON_MAX; i++) {
		bzero(href_buf, sizeof(href_buf));
		snprintf(href_buf, sizeof(href_buf), "href=%s", emotion_name_table[i].emoticon_name);
		emoticon = strstr(*src_text, href_buf);
		if (emoticon) {
			emoticon_pos = strlen(*src_text) - strlen(emoticon);

			if (emoticon_pos > start_item_pos && emoticon_pos < end_item_pos) {
				*src_text += start_item_pos;
				*item_length = end_item_pos - start_item_pos + 1;
				return i;
			}
		}
	}

	return ENTRY_EMOTICON_NONE;
}

char *entry_convert_emoticon_to_normal_text(const char *src_text)
{
	char *remain_text = (char *)src_text;
	char *dst_str = NULL;
	const char *str;

	Eina_Strbuf *msg_data = eina_strbuf_new();

	while (*remain_text) {
		char *text_start = remain_text;
		int emoticon = ENTRY_EMOTICON_NONE;
		int emoticon_txt_length = 0;

		emoticon = entry_emoticon_origin_string(&remain_text, &emoticon_txt_length);

		if (emoticon != ENTRY_EMOTICON_NONE) {
			eina_strbuf_append_length(msg_data, text_start, remain_text - text_start);
			eina_strbuf_append_printf(msg_data, "%s", emotion_name_table[emoticon].text);

			remain_text = remain_text + emoticon_txt_length;

			if (strncmp(remain_text, "</item>", strlen("</item>")) == 0) {
				remain_text = remain_text + strlen("</item>");
			}

			if (*remain_text == '\0')
				break;
		} else {
			eina_strbuf_append(msg_data, text_start);
			break;
		}
	}
	str = eina_strbuf_string_get(msg_data);
	if (str) dst_str = strdup(str);
	eina_strbuf_free(msg_data);

	return dst_str;
}

static char *efl_to_entry(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);

	char *emoticon_text = entry_convert_emoticon_to_normal_text(str);
	char *normal_text = markup_to_entry(ad, type_index, emoticon_text);

	return normal_text;
}

static char *image_path_to_entry(AppData *ad, int type_index, const char *str)
{
	CALLED();
	return NULL;
}

static char *text_to_entry(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	char *markup = NULL;
	char *emoticon_text = entry_convert_emoticon_to_normal_text(str);
	markup = (char*)evas_textblock_text_utf8_to_markup(NULL, emoticon_text);
	char *for_entry = markup_to_entry(ad, type_index, markup);
	FREE(markup);
	FREE(emoticon_text);
	return for_entry;
}

static Eina_List *make_tag_list(int type_index, const char *str)
{
	Eina_List *nodeList = NULL;
	PTagNode nodeData;

	nodeData = _get_start_node(str);

	while (nodeData)
	{
		nodeList = eina_list_append(nodeList, nodeData);
		nodeData = _get_next_node(nodeData);
	}

	_link_match_tags(nodeList);

	switch(type_index)
	{
		case ATOM_INDEX_EFL:
			_set_EFL_tag_data(nodeList);
			break;
		case ATOM_INDEX_HTML:
			_set_HTML_tag_data(nodeList);
			break;
		default:
			WRN("wrong index: %d", type_index);
	}

	_dumpNode(nodeList);
	return nodeList;
}

static void cleanup_tag_list(Eina_List *nodeList)
{
	Eina_List *trail;
	PTagNode nodeData;

	EINA_LIST_FOREACH(nodeList, trail, nodeData)
		_delete_node(nodeData);
	eina_list_free(nodeList);
}

static char *html_to_efl(AppData *ad, int type_index, const char *str)
{
	CALLED();
	Eina_List *nodeList = NULL;
	nodeList = make_tag_list(type_index, str);
	char *ret = _convert_to_edje(nodeList);
	DBG("efl: %s", ret);
	cleanup_tag_list(nodeList);

	return ret;
}

static char *efl_to_html(AppData *ad, int type_index, const char *str)
{
	CALLED();
	Eina_List *nodeList = NULL;
	nodeList = make_tag_list(type_index, str);
	char *ret = _convert_to_html(nodeList);
	DBG("html: %s", ret);
	cleanup_tag_list(nodeList);

	return ret;
}

static char *text_to_html(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	char *markup = NULL;
	markup = (char*)evas_textblock_text_utf8_to_markup(NULL, str);
	char *html = efl_to_html(ad, ATOM_INDEX_EFL, markup);
	FREE(markup);
	return html;
}

static char *text_to_efl(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	char *ret = NULL;
	char *emoticon_text = entry_convert_emoticon_to_normal_text(str);
	ret = (char*)evas_textblock_text_utf8_to_markup(NULL, emoticon_text);
	FREE(emoticon_text);
	return ret;
}

static char *to_text(AppData *ad, int type_index, const char *str)
{
	DBG("str: %s", str);
	char *entry_text = NULL;

	if (type_index == ATOM_INDEX_HTML)
	{
		Eina_Strbuf *buf = eina_strbuf_new();
		if (buf)
		{
			char *html;
			eina_strbuf_append(buf, str);
			eina_strbuf_replace_all(buf, "&nbsp;", " ");
			html = eina_strbuf_string_steal(buf);
			eina_strbuf_free(buf);
			entry_text = (char*)evas_textblock_text_markup_to_utf8(NULL, html);
			free(html);
			return entry_text;
		}
	}

	char *emoticon_text = entry_convert_emoticon_to_normal_text(str);
	if (emoticon_text)
	{
		char *tmp;
		tmp = markup_to_entry(ad, type_index, emoticon_text);
		entry_text = evas_textblock_text_markup_to_utf8(NULL, tmp);
		free(tmp);
		if (entry_text) strcat(entry_text, "\0");
		FREE(emoticon_text);
	}
	return entry_text;
}

