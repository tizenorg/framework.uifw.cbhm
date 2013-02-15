/*
 * cbhm
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */



#include "xconverter.h"

static char *html_to_entry(AppData *ad, int type_index, const char *str);
static char *efl_to_entry(AppData *ad, int type_index, const char *str);
static char *text_to_entry(AppData *ad, int type_index, const char *str);
static char *image_path_to_entry(AppData *ad, int type_index, const char *str);

static char *make_close_tag(Eina_List* nodes);
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
		1, 5, 2, 1, 2
	};
	char *targetAtomNames[][5] = {
		{ "TARGETS" },
		{ "UTF8_STRING", "STRING", "TEXT", "text/plain;charset=utf-8", "text/plain" },
		{ "text/html;charset=utf-8", "text/html" },
		{ "application/x-elementary-markup" },
		{ "text/uri", "text/uri-list" }
	};
	text_converter_func converts_for_entry[ATOM_INDEX_MAX] = {
		NULL, text_to_entry, html_to_entry, efl_to_entry, image_path_to_entry
	};

	text_converter_func converts[ATOM_INDEX_MAX][ATOM_INDEX_MAX] = {
		{NULL, NULL, NULL, NULL, NULL},
		{NULL, do_not_convert, text_to_html, text_to_efl, NULL},
		{NULL, to_text, do_not_convert, html_to_efl, NULL},
		{NULL, to_text, efl_to_html, do_not_convert, NULL},
		{NULL, NULL, image_path_to_html, image_path_to_efl, image_path_to_image_path}
	};

	int i, j;
	for (i = 0; i < ATOM_INDEX_MAX; i++)
	{
		ad->targetAtoms[i].atom_cnt = atom_cnt[i];
		ad->targetAtoms[i].name = MALLOC(sizeof(char *) * atom_cnt[i]);
		ad->targetAtoms[i].atom = MALLOC(sizeof(Ecore_X_Atom) * atom_cnt[i]);
		for (j = 0; j < atom_cnt[i]; j++)
		{
			DMSG("atomName: %s\n", targetAtomNames[i][j]);
			ad->targetAtoms[i].name[j] = strdup(targetAtomNames[i][j]);
			ad->targetAtoms[i].atom[j] = ecore_x_atom_get(targetAtomNames[i][j]);
		}
		ad->targetAtoms[i].convert_for_entry = converts_for_entry[i];

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
	if (item)
		item_type_index = item->type_index;

	for (i = 0, count = 0; i < ATOM_INDEX_MAX; i++)
	{
		if (ad->targetAtoms[item_type_index].convert_to_target[i])
			count += ad->targetAtoms[i].atom_cnt;
	}

	*data_ret = MALLOC(sizeof(Ecore_X_Atom) * count);
	DMSG("item_type: %d, target Atom cnt: %d\n", item_type_index, count);
	if (!*data_ret)
		return EINA_FALSE;

	for (i = 0, count = 0; i < ATOM_INDEX_MAX; i++)
	{
		if (ad->targetAtoms[item_type_index].convert_to_target[i])
		{
			for(j = 0; j < ad->targetAtoms[i].atom_cnt; j++)
			{
				((Ecore_X_Atom *)*data_ret)[count++] = ad->targetAtoms[i].atom[j];
				DMSG("send target atom: %s\n", ad->targetAtoms[i].name[j]);
			}
		}
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
		if (size_ret) *size_ret = strlen(*data_ret);
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

#ifdef DEBUG
static void _dumpNode(Eina_List* nodes);
#endif

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
		char *tagStart = strchr(str, '<');
		if (!tagStart)
			startStr = strdup(str);
		else
		{
			int strLength = tagStart - str;
			startStr = MALLOC(sizeof(char) * (strLength + 1));
			strncpy(startStr, str, strLength);
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
		tagStart = strchr(prev->pos_in_ori_str, '<');
	else
		tagStart = strchr(prev->pos_in_ori_str + 1, '<');

	if (!tagStart)
		return retTag;

	tagEnd = strchr(tagStart, '>');
	nextTagStart = strchr(tagStart + 1, '<');

	if (!tagEnd || (nextTagStart && (nextTagStart < tagEnd)))
		return _get_start_node(tagStart + 1);

	int spCnt = 5;
	char *spArray[spCnt];
	spArray[0] = strchr(tagStart, '=');
	spArray[1] = strchr(tagStart, '_');
	spArray[2] = strchr(tagStart, ' ');
	spArray[3] = strchr(tagStart, '\t');
	spArray[4] = strchr(tagStart, '\n');
	tagNameEnd = tagEnd;

	int i;
	for (i = 0; i < spCnt; i++)
	{
		if (spArray[i] && spArray[i] < tagNameEnd)
			tagNameEnd = spArray[i];
	}

	int tagLength = tagNameEnd - tagStart - 1;
	char *tagName = NULL;
	if (!strncmp(&tagStart[1], "/item", tagLength))
		tagName = strdup("");
	else
		tagName = strndup(&tagStart[1], tagLength);

	int tagStrLength = 0;
	char *tagStr = NULL;
	if (tagName)
	{
		tagStrLength = tagEnd - tagStart + 1;
		tagStr = strndup(tagStart, tagStrLength);
	}

	unsigned int strLength = nextTagStart ? (unsigned int)(nextTagStart - tagEnd - 1) : strlen(&tagEnd[1]);
	char *str = strndup(&tagEnd[1], strLength);

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
				if (!strcmp("font", node->tag))
				{
					PFontTagData data = node->tagData;
					SAFEFREE(data->name);
					SAFEFREE(data->color);
					SAFEFREE(data->size);
					SAFEFREE(data->bg_color);
				}
				if (!strcmp("item", node->tag))
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
		if (!trail->tag || trail->tag[0] == '\0')
			continue;
		if (!strcmp("br", trail->tag) || !strcmp("br/", trail->tag))
		{
			trail->tagPosType = TAGPOS_ALONE;
			continue;
		}
		else if (!strcmp("item", trail->tag) || !strcmp("img", trail->tag))
		{
			trail->tagPosType = TAGPOS_ALONE;
			continue;
		}

		if (trail->tag[0] != '/') // PUSH
		{
			stack = eina_list_append(stack, trail);
			/*             eina_array_push(stack, trail);
						   DMSG("stack: %d, tag %s\n", eina_array_count_get(stack), trail->tag);*/
			DMSG("stack: %d, tag %s\n", eina_list_count(stack), trail->tag);
		}
		else // POP
		{
			if (!eina_list_count(stack))
			{
				DMSG("tag not matched %s\n", trail->tag);
				continue;
			}

			EINA_LIST_REVERSE_FOREACH(stack, r, popData)
			{
				if (popData->tag && !strcmp(popData->tag, &trail->tag[1]))
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
						   DMSG("pop stack: %d, tag %s\n", eina_array_count_get(stack), trail->tag);
			 */
		}
	}

	/*   if (eina_array_count_get(stack))
		 DMSG("stack state: %d, tag %s\n", eina_array_count_get(stack), trail->tag);*/

	/* Make Dummy close tag */
	/*   while ((popData = eina_array_pop(stack)))  */

	EINA_LIST_REVERSE_FOREACH(stack, r, popData)
	{
		PTagNode newData;
		int tagLength = strlen(popData->tag);
		char *tagName = MALLOC(sizeof(char) * (tagLength + 2));

		tagName[0] = '/';
		tagName[1] = '\0';
		strcat(tagName, popData->tag);

		newData = _new_tag_node(tagName, NULL, NULL, NULL);
		popData->tagPosType = TAGPOS_START;
		newData->tagPosType = TAGPOS_END;
		popData->matchTag = newData;
		newData->matchTag = popData;
		nodes = eina_list_append(nodes, newData);
		/*        DMSG("stack: %d, tag %s\n", eina_array_count_get(stack), popData->tag);*/
	}
	/*   DMSG("stack_top: %d\n", eina_array_count_get(stack));
		 eina_array_free(stack);*/
	eina_list_free(stack);
}

static char *
_get_tag_value(const char *tag_str, const char *tag_name)
{
	if (!tag_name || !tag_str)
		return NULL;

	char *tag;
	if ((tag = strstr(tag_str, tag_name)))
	{
		if (tag[strlen(tag_name)] == '_')
			return NULL;
		char *value = strchr(tag, '=');
		if (value)
		{
			do
			{
				value++;
			} while (!isalnum(*value) && *value != '#');

			int spCnt = 6;
			char *spArray[spCnt];
			spArray[0] = strchr(value, ' ');
			spArray[1] = strchr(value, '>');
			spArray[2] = strchr(value, '\"');
			spArray[3] = strchr(value, '\'');
			spArray[4] = strchr(value, '\t');
			spArray[5] = strchr(value, '\n');
			char *valueEnd = strchr(value, '\0');

			int i;
			int start = 0;
			if ((!strncmp(tag_str, "<item", 5) && !strcmp(tag_name, "href")) // EFL img tag
               || (!strncmp(tag_str, "<img", 4) && !strcmp(tag_name, "src"))) // HTML img tag
               start = 1;

			for (i = start; i < spCnt; i++)
			{
				if (spArray[i] && spArray[i] < valueEnd)
					valueEnd = spArray[i];
			}

			int valueLength = valueEnd - value;
			return strndup(value, valueLength);
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
		char *path = strstr(value, "file://");
		if (path)
		{
			char *modify = MALLOC(sizeof(char) * (strlen(value) + 1));
			strncpy(modify, "file://", 8);
			path += 7;
			while (path[1] && path[0] && path[1] == '/' && path[0] == '/')
			{
				path++;
			}
			strcat(modify, path);
			data->href = modify;
			DMSG("image href ---%s---\n", data->href);
			FREE(value);
		}
		else
			freeAndAssign(data->href, value);
	}

	value = _get_tag_value(tag_str, "absize");
	if (value)
	{
		char *xpos = strchr(value, 'x');
		if (xpos)
		{
			int absizeLen = strlen(value);
			freeAndAssign(data->width, strndup(value, xpos - value));
			freeAndAssign(data->height, strndup(xpos + 1, absizeLen - (xpos - value) - 1));
			DMSG("image width: -%s-, height: -%s-\n", data->width, data->height);
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
		if (!trail->tag)
			continue;
		if (!strcmp("font", trail->tag) || !strcmp("color", trail->tag))
			trail->tagData = _set_EFL_font_data(trail->tagData, trail->tag_str);
		else if (!strcmp("item", trail->tag))
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
		char *path = strstr(value, "file://");
		if (path)
		{
			char *modify = MALLOC(sizeof(char) * (strlen(value) + 1));
			strncpy(modify, "file://", 8);
			path += 7;
			while (path[1] && path[0] && path[1] == '/' && path[0] == '/')
			{
				path++;
			}
			strcat(modify, path);
			data->href = modify;
			DMSG("image src ---%s---\n", data->href);
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
		if (!trail->tag)
			continue;
		if (!strcmp("font", trail->tag) || !strcmp("color", trail->tag))
			trail->tagData = _set_HTML_font_data(trail->tagData, trail->tag_str);
		else if (!strcmp("img", trail->tag))
			trail->tagData = _set_HTML_img_data(trail->tagData, trail->tag_str);
	}
}

#ifdef DEBUG
static void
_dumpNode(Eina_List* nodes)
{
	PTagNode trail;
	Eina_List *l;

	EINA_LIST_FOREACH(nodes, l, trail)
	{
		DMSG("tag: %s, tag_str: %s, str: %s, tagPosType: %d\n",
				trail->tag, trail->tag_str, trail->str, trail->tagPosType);
		DMSG("matchTag: %x ", (unsigned int)trail->matchTag);
		if (trail->matchTag)
			DMSG("matchTag->tag_str: %s", trail->matchTag->tag_str);
		if (trail->tagData)
		{
			if (!strcmp(trail->tag, "font"))
			{
				PFontTagData data = trail->tagData;
				DMSG(" tagData->name: %s, tagData->color: %s, tagData->size: %s, tagData->bg_color: %s",
						data->name, data->color, data->size, data->bg_color);
			}
			else if (!strcmp(trail->tag, "item") || !strcmp(trail->tag, "img"))
			{
				PItemTagData data = trail->tagData;
				DMSG(" tagData->href: %s, tagData->width: %s, tagData->height: %s",
						data->href, data->width, data->height);
			}
			else
				DMSG("\nERROR!!!! not need tagData");
		}
		DMSG("\n");
	}
}
#endif

static char *
_convert_to_html(Eina_List* nodes)
{
	PTagNode trail;
	Eina_List *l;

	Eina_Strbuf *html = eina_strbuf_new();

	int tableCnt = sizeof(_EFLtoHTMLConvertTable) / sizeof(TagTable);

	EINA_LIST_FOREACH(nodes, l, trail)
	{
		if (trail->tag)
		{
			char *tagName = trail->tagPosType == TAGPOS_END ?
				trail->matchTag->tag : trail->tag;
			int j;
			for(j = 0; j < tableCnt; j++)
			{
				if (!strcmp(_EFLtoHTMLConvertTable[j].src, tagName))
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
						if (!strcmp(_EFLtoHTMLConvertTable[j].src, "font"))
						{
							PFontTagData data = trail->tagData;
							if (data->name)
							{
							}
							if (data->color)
							{
								char *color = strdup(data->color);
								if (color && color[0] == '#' && strlen(color) == 9)
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
						else if (!strcmp(_EFLtoHTMLConvertTable[j].src, "item"))
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
		if (trail->tag)
		{
			char *tagName = trail->tagPosType == TAGPOS_END ?
				trail->matchTag->tag : trail->tag;
			int j;
			for(j = 0; j < tableCnt; j++)
			{
				if (!strcmp(_HTMLtoEFLConvertTable[j].src, tagName))
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
						if (!strcmp(_HTMLtoEFLConvertTable[j].src, "font"))
						{
							PFontTagData data = trail->tagData;
							if (data->name)
							{
							}
							if (data->color)
							{
								if (data->color[0] == '#' && strlen(data->color) == 7)
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
						else if (!strcmp(_HTMLtoEFLConvertTable[j].src, "img"))
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
							if (!strcmp(_HTMLtoEFLConvertTable[j].src, "font"))
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
	DMSG("type_index: %d ", type_index);
	DMSG("str: %s\n", str);
	if (ad->targetAtoms[type_index].convert_for_entry)
		return ad->targetAtoms[type_index].convert_for_entry(ad, type_index, str);
	return NULL;
}

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

static char *do_not_convert(AppData *ad, int type_index, const char *str)
{
	DMSG("str: %s\n", str);
	if (type_index != ATOM_INDEX_TEXT)
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

#ifdef DEBUG
		_dumpNode(nodeList);
#endif
		char *ret = make_close_tag(nodeList);
		cleanup_tag_list(nodeList);
		DMSG("convert str: %s\n", ret);
		return ret;
	}
	return strdup(str);
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
			DMSG("ERROR: wrong type_index: %d\n", type_index);
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
	DMSG("str: %s\n", str);
	return make_image_path_tag(ATOM_INDEX_TEXT, str);
}
*/

static char *image_path_to_html(AppData *ad, int type_index, const char *str)
{
	DMSG("str: %s\n", str);
	return make_image_path_tag(ATOM_INDEX_HTML, str);
}

static char *image_path_to_efl(AppData *ad, int type_index, const char *str)
{
	DMSG("str: %s\n", str);
	return make_image_path_tag(ATOM_INDEX_EFL, str);
}

static char *image_path_to_image_path(AppData *ad, int type_index, const char *str)
{
	DMSG("str: %s\n", str);
	return make_image_path_tag(ATOM_INDEX_IMAGE, str);;
}
static char *markup_to_entry(AppData *ad, int type_index, const char *str)
{
	CALLED();
	if (!str)
		return NULL;

	Eina_Strbuf *strbuf = eina_strbuf_new();
	if (!strbuf)
		return strdup(str);
	eina_strbuf_prepend(strbuf, "<font_size=28><color=#000000FF>");

	const char *trail = str;
	char *image_tag_str = NULL;
	char *html_img_tag = "img";
	char *efl_img_tag = "item";
	if (type_index == ATOM_INDEX_HTML) /* HTML */
		image_tag_str = html_img_tag;
	else if (type_index == ATOM_INDEX_EFL) /* EFL */
		image_tag_str = efl_img_tag;

	while (trail && *trail)
	{
		const char *pretrail = trail;
		unsigned long length;
		char *temp;
		char *endtag;

		trail = strchr(trail, '<');
		if (!trail)
		{
			eina_strbuf_append(strbuf, pretrail);
			break;
		}
		endtag = strchr(trail, '>');
		if (!endtag)
			break;

		length = trail - pretrail;

		temp = strndup(pretrail, length);
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

		if (strncmp(trail, "br", 2) == 0)
		{
			eina_strbuf_append(strbuf, "<br>");
			trail = endtag + 1;
			continue;
		}

		if (image_tag_str && strncmp(trail, image_tag_str, strlen(image_tag_str)) == 0)
		{
			char *src = strstr(trail, "file://");
			char *src_endtag = strchr(trail, '>');
			if (!src || !src_endtag || src_endtag < src)
				continue;

			length = src_endtag - src;

			src = strndup(src, length);
			if (!src)
			{
				trail = endtag + 1;
				continue;
			}
			temp = src;
			while(*temp)
			{
				if (*temp == '\"' || *temp == '>')
					*temp = '\0';
				else
					temp++;
			}

			eina_strbuf_append_printf(strbuf, "<item absize=66x62 href=%s></item>", src);
			DTRACE("src str: %s \n", src);
			FREE(src);
		}
		trail = endtag + 1;
	}

	if (type_index == ATOM_INDEX_HTML)
		eina_strbuf_replace_all(strbuf, "&nbsp;", " ");

	char *entry_str = eina_strbuf_string_steal(strbuf);
	eina_strbuf_free(strbuf);
	return entry_str;
}

static char *html_to_entry(AppData *ad, int type_index, const char *str)
{
	DMSG("str: %s\n", str);
	return markup_to_entry(ad, type_index, str);
}

static char *efl_to_entry(AppData *ad, int type_index, const char *str)
{
	DMSG("str: %s\n", str);
	return markup_to_entry(ad, type_index, str);
}

static char *image_path_to_entry(AppData *ad, int type_index, const char *str)
{
	CALLED();
	return NULL;
}

static char *text_to_entry(AppData *ad, int type_index, const char *str)
{
	DMSG("str: %s\n", str);
	char *markup = NULL;
	markup = (char*)_elm_util_text_to_mkup(str);
	char *for_entry = markup_to_entry(ad, type_index, markup);
	FREE(markup);
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
			DMSG("wrong index: %d\n");
	}

#ifdef DEBUG
	_dumpNode(nodeList);
#endif
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
	DMSG("efl: %s\n", ret);
	cleanup_tag_list(nodeList);

	return ret;
}

static char *efl_to_html(AppData *ad, int type_index, const char *str)
{
	CALLED();
	Eina_List *nodeList = NULL;
	nodeList = make_tag_list(type_index, str);
	char *ret = _convert_to_html(nodeList);
	DMSG("html: %s\n", ret);
	cleanup_tag_list(nodeList);

	return ret;
}

static char *text_to_html(AppData *ad, int type_index, const char *str)
{
	DMSG("str: %s\n", str);
	char *markup = NULL;
	markup = (char*)_elm_util_text_to_mkup(str);
	char *html = efl_to_html(ad, ATOM_INDEX_EFL, markup);
	FREE(markup);
	return html;
}

static char *text_to_efl(AppData *ad, int type_index, const char *str)
{
	DMSG("str: %s\n", str);
	char *ret = NULL;
	ret = (char*)_elm_util_text_to_mkup(str);
	return ret;
}

static char *to_text(AppData *ad, int type_index, const char *str)
{
	DMSG("str: %s\n", str);
	char *text = NULL;
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
			text = (char*)_elm_util_mkup_to_text(html);
			free(html);
			return text;
		}
	}

	text = (char*)_elm_util_mkup_to_text(str);
	return text;
}

