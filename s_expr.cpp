// this file is part of osmbrowser
// copyright Martijn Versteegh
// osmbrowser is licenced under the gpl v3
#include "s_expr.h"


char const *Type::s_typeNames[] =
{
	"node",
	"way",
	"relation"
};

unsigned Type::s_numTypes = sizeof(Type::s_typeNames) / sizeof(char *);


Operators::E_OPERATOR ExpressionParser::MatchOperator(char const *s, int *pos, bool *disabled)
{
	char const *operators[] =
	{
		"not", "and", "or", "tag", "type"
	};

	if (s[*pos] == '-')
	{
		*disabled = true;
		(*pos)++;
		m_mustColorDisabled++;
	}

	int count = sizeof(operators)/sizeof(char *);
	for (int i = 0; i < count ; i++)
	{
		if (!strncasecmp(operators[i], s + *pos, strlen(operators[i])))
		{
			int len = strlen(operators[i]);
			SetColorD(*pos, *pos + len, RuleDisplay::EC_OPERATOR);
			*pos += len;
			return static_cast<Operators::E_OPERATOR>(i);
		}
	}

	return static_cast<Operators::E_OPERATOR>(count);
}


char *ExpressionParser::ParseString(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos)
{
	static char buffers[2][1024];
	static int curBuffer = 0;

	curBuffer++;
	curBuffer %= 2;

	EatSpace(f, pos);

	int p = *pos;

	if (f[p] != '"' && f[p] != '\'')
	{
		snprintf(logError, maxLogErrorSize, "expected string value");
		*errorPos= p;
		return NULL;
	}

	int end = f[p];

	p++;
	int i = 0;
	while (f[p] && f[p] != end)
	{
		buffers[curBuffer][i++] = f[p++];

		if (i >= 1023)
			i = 1023;
	}

	buffers[curBuffer][i] = 0;

	if (f[p] != end)
	{
		snprintf(logError, maxLogErrorSize, "expected %c for end of string", end);
		*errorPos= p;
		return NULL;
	}

	p++;

	SetColorD(*pos, p, RuleDisplay::EC_STRING);

	*pos = p;

	EatSpace(f,pos);
    
	return buffers[curBuffer];
}


unsigned ExpressionParser::ParseMultiple(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos, LogicalExpression *parent)
{
	LogicalExpression *ret = ParseSingle(f, pos, logError, maxLogErrorSize, errorPos);

	if (!ret)
	{
		return 0;
	}

	parent->AddChild(ret);

	LogicalExpression *next;

	while (true)
	{
		EatSpace(f, pos);

		if (f[*pos] != '(')
		{
			break;
		}

		next = ParseSingle(f, pos, logError, maxLogErrorSize, errorPos);

		if (!next)
		{
			parent->ClearChildren();
			return 0;
		}

		parent->AddChild(next);
	}

	return parent->GetNumChildren();
}

void ExpressionParser::EatSpace(char const *s, int *pos)
{
	int p = *pos;

	while (s[p] && isspace(s[p]))
	{
		p++;
	}

	SetColorD(*pos, p, RuleDisplay::EC_SPACE);

	*pos = p;
}


LogicalExpression *ExpressionParser::ParseSingle(char const *f, int *pos, char *logError, unsigned maxLogErrorSize, unsigned *errorPos)
{
	bool disabled = false;
	LogicalExpression *ret = NULL;
	LogicalExpression *c = NULL;
	Operators::E_OPERATOR op = Operators::INVALID;

	EatSpace(f, pos);

	int p = *pos;

	if (!(f[p]))
	{
		snprintf(logError, maxLogErrorSize, "unexpected end of expression");
		goto error;
		
	}

	if (f[p] != '(')
	{
		snprintf(logError, maxLogErrorSize, "expected '('");
		goto error;
	}

	SetColorD(p, p+1, RuleDisplay::EC_BRACKET);

	p++;


	op = MatchOperator(f, &p, &disabled);

	switch(op)
	{
		case Operators::NOT: // not
			ret = new Not;
		break;
		case Operators::AND:  // and
			ret = new And;
		break;
		case Operators::OR:
			ret = new Or;
		break;
		case Operators::TYPE:
		{
			Type::TYPE t = Type::GetType(ParseString(f, &p,logError, maxLogErrorSize, errorPos));

			if (t == Type::INVALID)
			{
				snprintf(logError, maxLogErrorSize, "invalid type");
				goto error;
			}

			ret = new Type(t);
		}
		break;
		case Operators::TAG:
		{
			char const *key = ParseString(f, &p,logError, maxLogErrorSize, errorPos);
			char const *value = ParseString(f, &p,logError, maxLogErrorSize, errorPos);

			if (!key)
			{
				snprintf(logError, maxLogErrorSize, "expected tag key");
				goto error;
			}
			
//			if (!OsmTag::KeyExists(key))
//			{
//				snprintf(logError, maxLogErrorSize, "unknown tag key '%s'", key);
//				goto error;
//			}

			Tag *tag = new Tag(key, value);

			ret = tag;
			if (value)	// if we had one value, try to see if there are more values specified and build an "or" expression of multiple tags if we do
			{
				value = ParseString(f, &p,logError, maxLogErrorSize, errorPos);
				if (value)
				{
					LogicalExpression *orExpr = new Or;
					orExpr->AddChild(tag);
					orExpr->AddChild(new Tag(tag->Key(), value));

					while ((value = ParseString(f, &p,logError, maxLogErrorSize, errorPos)))
					{
						orExpr->AddChild(new Tag(tag->Key(), value));
					}

					ret = orExpr;
				}
			}

		}
		break;
		default:
			snprintf(logError, maxLogErrorSize, "unknown operator");
			goto error;
		break;
	}

	switch(op)
	{
		case Operators::NOT:
			c = ParseSingle(f, &p, logError, maxLogErrorSize, errorPos);
			if (!c)
			{
//				snprintf(logError, maxLogErrorSize, "expect subexpression");
				goto error;
			}
		break;
		case Operators::AND:
		//fallthrough
		case Operators::OR:
		{
			unsigned howMany = ParseMultiple(f, &p, logError, maxLogErrorSize, errorPos, ret);
			if (!howMany)
			{
//				snprintf(logError, maxLogErrorSize, "expect subexpression(s)");
				goto error;
			}
		}
		break;
		case Operators::TAG:
		break;
		default:
		break;
	}


	if (c)
	{
		ret->AddChild(c);
	}

	EatSpace(f, &p);

	if (f[p] != ')')
	{
		snprintf(logError, maxLogErrorSize, "expected ')'");
		*errorPos = p;
		goto error;
	}

	SetColorD(p, p+1, RuleDisplay::EC_BRACKET);

	p++;
	*pos = p;
	// clear the errorlog
	*logError = 0;

	ret->m_disabled = disabled;

	if (disabled)
	{
		m_mustColorDisabled--;
	}

	return ret;
	error:

	if (disabled)
	{
		m_mustColorDisabled--;
	}

	if (ret)
	{
		delete ret;
	}
	
	*errorPos = p;

	SetColorD(p, strlen(f), RuleDisplay::EC_ERROR);
	return NULL;
	

}

