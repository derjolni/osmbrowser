// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#ifndef __S_EXPRESSION_H__
#define __S_EXPRESSION_H__
#include <wx/dynarray.h>

#include <ctype.h>
/*
  e =
  (type relation|way|node)
  | (tag key)                      true if a tag with this key exists
  | (tag key value)              true if the key/value pair exists
  | (and e e e e e ...)          true if all e's true
  | (or e e e e ...)             true if any e true
  | (not e)                      true if e false and vv
  | (identity e)                 true if e is true (to esily change a not without changing the whole tree
  
  
*/

#include "osm.h"
#include "external-libs/md5/md5.h"
// an MD5 class geared to comparing LogicalExpression instances
class ExpressionMD5
{
	public:
		ExpressionMD5()
		{
			m_inited = false;
			m_finished = false;
			memset(m_digest, 0, 16);
		}

		void Add(ExpressionMD5 const &other)
		{
			assert(m_inited);
			assert(other.Finished());
			Add(other.m_digest, 16);
		}

		void Add(void const *data, int num)
		{
			assert(m_inited);
			assert(!m_finished);
			assert(sizeof(md5_byte_t) == sizeof(char));
			md5_append(&m_md5, (md5_byte_t *)data, num);
		}
		void Finish()
		{
			assert(m_inited);
			assert(!m_finished);
			assert(sizeof(md5_byte_t) == sizeof(char));
			md5_finish(&m_md5, (md5_byte_t *)m_digest);
			m_finished = true;
			m_inited = false;
		}

		bool Finished() const
		{
			return m_finished;
		}

		ExpressionMD5 const &operator=(ExpressionMD5 const &other)
		{
			m_finished = other.m_finished;
			m_inited = other.m_inited;
			memcpy(m_digest, other.m_digest, 16);
			return *this;
		}

		// difference for sorting
		int Difference(ExpressionMD5 const &other) const
		{
			if (!m_finished && !other.m_finished)
			{
				return 0;
			}

			if (!m_finished)
			{
				return 1;
			}
			else if (!other.m_finished)
			{
				return -1;
			}

			for (int i = 0; i < 16; i++)
			{
				if (m_digest[i] != other.m_digest[i])
				{
					return m_digest[i] - other.m_digest[i];
				}
			}

			return 0;
		}

		void Dump() const
		{
			printf("%c%c", m_inited ? 't' : 'f' ,m_finished ? 't' : 'f');
			for (int i = 0; i < 16 ; i++)
			{
				printf("%02X", m_digest[i]);
			}
		}

	private:
		friend class LogicalExpression;
		void Init()
		{
			assert(!m_inited);
			md5_init(&m_md5);
			m_inited = true;
		}
		md5_state_t m_md5;
		bool m_finished, m_inited;
		char m_digest[16];
};

class LogicalExpression;

WX_DEFINE_ARRAY_PTR(LogicalExpression *, LogicalExpressionArray);

class LogicalExpression
{
	public:
		LogicalExpression()
		{
			m_disabled = false;
		}
		virtual ~LogicalExpression()
		{
			WX_CLEAR_ARRAY(m_children);
		}

		enum STATE
		{
			S_FALSE,
			S_TRUE,
			S_IGNORE
//			S_INVALID
		};

		virtual bool Valid() const = 0;

		void AddChild(LogicalExpression *c)
		{
			m_children.Add(c);
		}

		void ClearChildren()
		{
			WX_CLEAR_ARRAY(m_children);
		}

		unsigned GetNumChildren()
		{
			return m_children.GetCount();
		}

		LogicalExpressionArray m_children;
		virtual STATE GetValue(IdObjectWithTags *o) = 0;
		virtual void Reorder() = 0;
		bool Same(LogicalExpression const *other)
		{
			if (!other)
			{
				return false;
			}
			return !(m_md5.Difference(other->m_md5));
		}

		ExpressionMD5 const &MD5() const
		{
			m_md5.Init();
			char flags = m_disabled ? 1 : 0;
			m_md5.Add(&flags, sizeof(flags));
			CalcMD5();
			return m_md5;
		}
		bool m_disabled;

		virtual void Dump(int indent) const = 0;
	protected:
		virtual void CalcMD5() const = 0;
		mutable ExpressionMD5 m_md5;

};


int CompareLogicalExpressionPtrs(LogicalExpression **p1, LogicalExpression **p2);


class Operators
{
public:
	enum E_OPERATOR
	{
		NOT,
		AND,
		OR,
		TAG,
		TYPE,
		OFF,
		INVALID
	};

};

class Type
	: public LogicalExpression
{
	public:
		enum TYPE
		{
			NODE,
			WAY,
			RELATION,
			INVALID
		};

		static char const *s_typeNames[];
		static unsigned s_numTypes;

		void Dump(int indent) const
		{
			for (int i = 0; i < indent; i++)
				printf(" ");
			m_md5.Dump();
			printf(" (type \"%s\")\n", s_typeNames[m_type]);
		}

		static TYPE GetType(char const *s)
		{
			if (!s)
			{
				return INVALID;
			}


			for (unsigned i = 0; i < s_numTypes; i++)
			{
				if (!strcmp(s, s_typeNames[i]))
				{
					return (Type::TYPE)i;
				}
			}

			return Type::INVALID;
		}

		bool Valid() const
		{
			return m_type != Type::INVALID;
		}

		void Reorder()
		{
			// nothing to do
		}

		Type(TYPE type)
		{
			m_type = type;
		}
		virtual STATE GetValue(IdObjectWithTags *o)
		{
			if (m_disabled)
				return S_IGNORE;

			switch(m_type)
			{
				case NODE:
					return dynamic_cast<OsmNode *>(o) == NULL ? S_FALSE : S_TRUE;
				break;
				case RELATION:
					return dynamic_cast<OsmRelation *>(o) == NULL ? S_FALSE : S_TRUE;
				break;
				case WAY:
					return ((dynamic_cast<OsmWay *>(o) != NULL)  && (dynamic_cast<OsmRelation *>(o) == NULL)) ? S_TRUE : S_FALSE;
				break;
				case INVALID:
					return S_IGNORE;
				break;
			}

			return S_IGNORE;
		}

	protected:
		void CalcMD5() const
		{
			int op = (int)(Operators::TYPE);
			m_md5.Add(&op, sizeof(op));
			m_md5.Add(&m_type, sizeof(m_type));
			m_md5.Finish();
		}
	private:
		TYPE m_type;
		Type() {} // don't use plz
};

class Not
	: public LogicalExpression
{
	public:
		virtual STATE GetValue(IdObjectWithTags *o)
		{
			if (m_disabled)
				return S_IGNORE;

			STATE states[] = {  S_TRUE, S_FALSE, S_IGNORE};
			STATE s = m_children[0]->GetValue(o);

			return states[s];
		}

		void Dump(int indent) const
		{
			for (int i = 0; i < indent; i++)
			{
				printf(" ");
			}
			m_md5.Dump();
			printf(" (not\n");
			m_children[0]->Dump(indent+1);
			for (int i = 0; i < indent; i++)
			{
				printf(" ");
			}
			printf(")");
		}

		bool Valid() const
		{
			return m_children[0]->Valid();
		}

		void Reorder()
		{
			// nothing to do
		}

		void CalcMD5() const
		{
			int op = (int)(Operators::NOT);
			m_md5.Add(&op, sizeof(op));
			m_md5.Add(m_children[0]->MD5());
			m_md5.Finish();
		}


};

class And
	: public LogicalExpression
{
	public:
		STATE GetValue(IdObjectWithTags *o)
		{
			if (m_disabled)
				return S_IGNORE;
		
			int trueCount = 0;
			for (unsigned i = 0; i < m_children.GetCount(); i++)
			{
				if (!m_children[i]->m_disabled)
				{
					STATE s = m_children[i]->GetValue(o);
					if (s == S_FALSE)
						return S_FALSE;
					else if (s == S_TRUE)
						trueCount++;

				}
				
			}

			return trueCount ? S_TRUE : S_IGNORE;
		}

		void Dump(int indent) const
		{
			for (int i = 0; i < indent; i++)
			{
				printf(" ");
			}
			m_md5.Dump();
			printf(" (and\n");
			for (unsigned i = 0 ; i < m_children.GetCount(); i++)
			{
				m_children[i]->Dump(indent+1);
			}
			for (int i = 0; i < indent; i++)
			{
				printf(" ");
			}
			printf(")\n");
		}


		bool Valid() const
		{
			for (unsigned i = 0; i < m_children.GetCount(); i++)
			{
				if (!(m_children[i]->Valid()))
				{
					return false;
				}
			}

			return true;
		}

		void Reorder()
		{
			for (unsigned i = 0; i < m_children.GetCount(); i++)
			{
				m_children[i]->Reorder();
			}

			// now that all children are in a standardized form, reorder the children
			m_children.Sort(CompareLogicalExpressionPtrs);
		}

		void CalcMD5() const
		{
			int op = (int)(Operators::AND);
			m_md5.Add(&op, sizeof(op));
			for (unsigned i = 0; i < m_children.GetCount(); i++)
			{
				m_md5.Add(m_children[i]->MD5());
			}
			m_md5.Finish();
		}


};

class Or
	: public LogicalExpression
{
	public:
		STATE GetValue(IdObjectWithTags *o)
		{
			if (m_disabled)
				return S_IGNORE;

			int falseCount = 0;
			for (unsigned i = 0; i < m_children.GetCount(); i++)
			{
				if ( !m_children[i]->m_disabled)
				{
					STATE s = m_children[i]->GetValue(o);

					switch(s)
					{
						case S_TRUE:
							return S_TRUE;
							break;
						case S_FALSE:
							falseCount++;
							break;
						default:
							break;
					}
				}
			}

			return falseCount ? S_FALSE : S_IGNORE;
		}

		bool Valid() const
		{
			for (unsigned i = 0; i < m_children.GetCount(); i++)
			{
				if (!(m_children[i]->Valid()))
				{
					return false;
				}
			}

			return true;
		}


		void Dump(int indent) const
		{
			for (int i = 0; i < indent; i++)
			{
				printf(" ");
			}
			m_md5.Dump();
			printf(" (or\n");
			for (unsigned i = 0 ; i < m_children.GetCount(); i++)
			{
				m_children[i]->Dump(indent+1);
			}
			for (int i = 0; i < indent; i++)
			{
				printf(" ");
			}
			printf(")\n");
		}

		void Reorder()
		{
			for (unsigned i = 0; i < m_children.GetCount(); i++)
			{
				m_children[i]->Reorder();
			}

			// now that all children are in a standardized form, reorder the children
			m_children.Sort(CompareLogicalExpressionPtrs);
		}

		void CalcMD5() const
		{
			int op = (int)(Operators::OR);
			m_md5.Add(&op, sizeof(op));
			for (unsigned i = 0; i < m_children.GetCount(); i++)
			{
				m_md5.Add(m_children[i]->MD5());
			}
			m_md5.Finish();
		}

};


class Tag
	: public LogicalExpression
{
	public:
		Tag(char const *key, char const *value)
		{
			m_tag = new OsmTag(true, key, value);
		}

		~Tag()
		{
			delete m_tag;
		}


		void Dump(int indent) const
		{
			for (int i = 0; i < indent; i++)
				printf(" ");
			m_md5.Dump();
			printf(" (tag %d %d)\n", m_tag->m_index.m_keyIndex, m_tag->m_index.m_valueIndex);
		}

		bool Valid() const
		{
			return true;
		}

		void Reorder()
		{
			// nothing to do
		}

		char const *Key() const
		{
			return m_tag->GetKey();
		}

		STATE GetValue(IdObjectWithTags *o)
		{
			if (m_disabled)
				return S_IGNORE;
			return o->HasTag(*m_tag) ? S_TRUE : S_FALSE;
		}

		void CalcMD5() const
		{
			int op = (int)(Operators::TAG);
			m_md5.Add(&op, sizeof(op));
			TagIndex index = m_tag->Index();
			m_md5.Add(&(index.m_keyIndex), sizeof(index.m_keyIndex));
			m_md5.Add(&(index.m_valueIndex), sizeof(index.m_valueIndex));
			m_md5.Finish();
		}

	private:
		OsmTag *m_tag;

};

class RuleDisplay
{
	public:
		enum E_COLORS
		{
			EC_SPACE,
			EC_BRACKET,
			EC_OPERATOR,
			EC_STRING,
			EC_ERROR,
			EC_DISABLED
		};

		virtual void SetColor(int from, int to, E_COLORS color) = 0;
};

class ExpressionParser
{
	public:
		ExpressionParser()
		{
			m_mustColorDisabled = 0;
		}

		LogicalExpression *Parse(char const *from, char *logError, unsigned maxLogErrorSize, unsigned *errorPos, RuleDisplay *display = NULL)
		{
			int pos = 0;
			m_display = display;
			return ParseSingle(from, &pos, logError, maxLogErrorSize, errorPos);
		}

	private:
		RuleDisplay *m_display;

		void EatSpace(char const *s, int *pos);

		Operators::E_OPERATOR MatchOperator(char const *s, int *pos, bool *disabled);
		
		char *ParseString(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos);
		
		unsigned ParseMultiple(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos, LogicalExpression *parent);

		LogicalExpression *ParseSingle(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos);


		void SetColorD(int from, int to, RuleDisplay::E_COLORS color)
		{
			if (!m_display)
			{
				return;
			}
			
			if (m_mustColorDisabled && color != RuleDisplay::EC_ERROR)
			{
				m_display->SetColor(from, to, RuleDisplay::EC_DISABLED);
			}
			else
			{
				m_display->SetColor(from, to, color);
			}
			
		}

		int m_mustColorDisabled;

        
};

class Rule
	: public ExpressionParser
{
	public:

		Rule(wxString const &text, RuleDisplay *display = NULL)
		{
			m_expr = NULL;
			SetRule(text, display);
		}
	
		Rule()
		{
			m_expr = NULL;
			m_errorPos = 0;
		}

		bool Differs(Rule const &other) const
		{
			if (!m_expr && ! other.m_expr)
			{
				return false;
			}

			if (!m_expr)
			{
				return true;
			}

			return !m_expr->Same(other.m_expr);
		}

		void Dump() const
		{
			printf("dump rule %p  -----------\n", this);
			if (!m_expr)
			{
				printf("(no rule)\n");
			}
			else
			{
				m_expr->MD5(); // force calculating of MD5s
				m_expr->Dump(0);
			}
			printf("-----------\n");

		}


		Rule(Rule const &other)
		{
			m_expr = NULL;
			Create(other);
		}

		Rule const &operator=(Rule const &other)
		{
			Create(other);

			return *this;
		}

		~Rule()
		{
			delete m_expr;
		}
		// set a new ruletext. returns true if the text is a valid expression
		bool SetRule(wxString const &text, RuleDisplay *display = NULL)
		{
			delete m_expr;
			m_text = text;

			char errorLog[1024] = {0};
			m_expr = Parse(text.mb_str(wxConvUTF8),  errorLog, 1024, &m_errorPos, display);

			m_errorLog = wxString::FromUTF8(errorLog);

			if (m_expr && m_expr->Valid())
			{
				m_expr->Reorder(); // use a standard ordering, to make comparing easier
			}
			return m_expr;
		}


		bool IsValid() { return m_expr && m_expr->Valid(); }
		
		wxString const &GetErrorLog()
		{
			return m_errorLog;
		}
		
		unsigned int GetErrorPos()
		{
			return m_errorPos;
		}

		bool Valid()
		{
			return m_expr && m_expr->Valid();
		}

		ExpressionMD5 const &MD5()
		{
			static ExpressionMD5 s_empty;
			return m_expr ? m_expr->MD5() : s_empty;
		}

		LogicalExpression::STATE Evaluate(IdObjectWithTags *o)
		{
			assert(Valid());
			if (!m_expr)
			{
				return LogicalExpression::S_IGNORE;
			}

			return m_expr->GetValue(o);
		}
	private:
		void Create(Rule const &other)
		{
			SetRule(other.m_text);
		}
		
		LogicalExpression *m_expr;
		wxString m_text;
		wxString m_errorLog;
		unsigned int m_errorPos;
		
};


#endif

