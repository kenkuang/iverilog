/*
 * Copyright (c) 2001 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#ifdef HAVE_CVS_IDENT
#ident "$Id: vpi_const.cc,v 1.24 2003/03/10 23:37:07 steve Exp $"
#endif

# include  "vpi_priv.h"
# include  <stdio.h>
#ifdef HAVE_MALLOC_H
# include  <malloc.h>
#endif
# include  <stdlib.h>
# include  <string.h>
# include  <assert.h>

static char buf[4096];

static int string_get(int code, vpiHandle ref)
{
    struct __vpiStringConst*rfp;

      switch (code) {
          case vpiSize:
	      rfp = (struct __vpiStringConst*)ref;
	      assert(ref->vpi_type->type_code == vpiConstant);

	      //fprintf(stderr, "String:|%s|, Length: %d\n", rfp->value, strlen(rfp->value));
	      return strlen(rfp->value)*8;

          case vpiSigned:
	      return 0;

	  case vpiConstType:
	      return vpiStringConst;

	  default:
	      fprintf(stderr, "vvp error: get %d not supported "
		      "by vpiStringConst\n", code);
	      assert(0);
	      return 0;
      }
}

static void string_value(vpiHandle ref, p_vpi_value vp)
{
      int size;
      unsigned uint_value;
      char *cp;

      struct __vpiStringConst*rfp = (struct __vpiStringConst*)ref;
      assert((ref->vpi_type->type_code == vpiConstant)
	     || ((ref->vpi_type->type_code == vpiParameter)));

      switch (vp->format) {
	  case vpiObjTypeVal:
	  case vpiStringVal:
	    vp->value.str = (char*)rfp->value;
	    vp->format = vpiStringVal;
	    break;

          case vpiDecStrVal:
	      size = strlen(rfp->value);
	      if (size > 4){
		  // We only support standard integers. Ignore other bytes...
		  size = 4;	
		  fprintf(stderr, "Warning (vpi_const.cc): %%d on constant strings only looks "
			  "at first 4 bytes!\n");
	      }

	      uint_value = 0;
	      for(int i=0; i<size;i ++){
		  uint_value <<=8;
		  uint_value += (unsigned char)(rfp->value[i]);
	      }

	      sprintf(buf, "%u", uint_value);

	      vp->format = vpiDecStrVal;
	      vp->value.str = buf;
	      break;

          case vpiBinStrVal:
	      size = strlen(rfp->value);
	      if (size*8 > (int)(sizeof(buf)/sizeof(char))-1 ){
		  // Avoid overflow of 'buf'
		  // 4096 should be sufficient for most cases though. ;-)
		  size = (sizeof(buf)/sizeof(char)-2)/8;	
	      }

	      cp = buf;
	      for(int i=0; i<size;i ++){
		  for(int bit=7;bit>=0; bit--){
		      *cp++ = "01"[ (rfp->value[i]>>bit)&1 ];
		  }
	      }
	      *cp = 0;

	      vp->format = vpiBinStrVal;
	      vp->value.str = buf;
	      break;

          case vpiHexStrVal:
	      size = strlen(rfp->value);
	      if (size*2 > (int)(sizeof(buf)/sizeof(char))-1 ){
		  // Avoid overflow of 'buf'
		  // 4096 should be sufficient for most cases though. ;-)
		  size = (sizeof(buf)/sizeof(char)-2)/2;	
	      }

	      cp = buf;
	      for(int i=0; i<size;i++){
		  for(int nibble=1;nibble>=0; nibble--){
		      *cp++ = "0123456789abcdef"[ (rfp->value[i]>>(nibble*4))&15 ];
		  }
	      }
	      *cp = 0;

	      vp->format = vpiHexStrVal;
	      vp->value.str = buf;
	      break;

          case vpiOctStrVal:
	      fprintf(stderr, "ERROR (vpi_const.cc): %%o display of constant strings not yet implemented\n");
	      assert(0);
	      break;

	  default:
	    fprintf(stderr, "ERROR (vpi_const.cc): vp->format: %d\n", vp->format);
	    assert(0);

	    vp->format = vpiSuppressVal;
	    break;
      }
}

static const struct __vpirt vpip_string_rt = {
      vpiConstant,
      string_get,
      0,
      string_value,
      0,
      0,
      0
};

static int free_temp_string(vpiHandle obj)
{
      struct __vpiStringConst*rfp = (struct __vpiStringConst*)obj;
      assert(obj->vpi_type->type_code == vpiConstant);

      free(rfp->value);
      free(rfp);
      return 1;
}

static const struct __vpirt vpip_string_temp_rt = {
      vpiConstant,
      string_get,
      0,
      string_value,
      0,

      0,
      0,
      0,

      free_temp_string
};


vpiHandle vpip_make_string_const(char*text, bool persistent_flag)
{
      struct __vpiStringConst*obj;

      obj = (struct __vpiStringConst*)
	    malloc(sizeof (struct __vpiStringConst));
      obj->base.vpi_type = persistent_flag
	    ? &vpip_string_rt
	    : &vpip_string_temp_rt;
      obj->value = text;

      return &obj->base;
}

    
struct __vpiStringParam  : public __vpiStringConst {
      const char*basename;
};

static char* string_param_get_str(int code, vpiHandle obj)
{
      struct __vpiStringParam*rfp = (struct __vpiStringParam*)obj;
      assert(obj->vpi_type->type_code == vpiParameter);

      switch (code) {
	  case vpiName:
	    return const_cast<char*>(rfp->basename);
	  default:
	    return 0;
      }
}

static const struct __vpirt vpip_string_param_rt = {
      vpiParameter,
      string_get,
      string_param_get_str,
      string_value,
      0,

      0,
      0,
      0,

      0
};


vpiHandle vpip_make_string_param(char*name, char*text)
{
      struct __vpiStringParam*obj;

      obj = (struct __vpiStringParam*)
	    malloc(sizeof (struct __vpiStringParam));
      obj->base.vpi_type = &vpip_string_param_rt;
      obj->value = text;
      obj->basename = name;

      return &obj->base;
}

static int binary_get(int code, vpiHandle ref)
{
      struct __vpiBinaryConst*rfp = (struct __vpiBinaryConst*)ref;
      assert(ref->vpi_type->type_code == vpiConstant);

      switch (code) {
	  case vpiConstType:
	    return vpiBinaryConst;

	  case vpiSigned:
	    return rfp->signed_flag? 1 : 0;

	  case vpiSize:
	    return rfp->nbits;

	  default:
	    fprintf(stderr, "vvp error: get %d not supported "
		    "by vpiBinaryConst\n", code);
	    assert(0);
	    return 0;
      }
}

static void binary_vpiStringVal(struct __vpiBinaryConst*rfp, p_vpi_value vp)
{
      unsigned nchar = rfp->nbits / 8;
      unsigned tail = rfp->nbits%8;

      char*cp = buf;

      if (tail > 0) {
	    char char_val = 0;
	    for (unsigned idx = rfp->nbits-tail; idx < rfp->nbits; idx += 1) {
		  unsigned nibble = idx/4;
		  unsigned shift  = 2 * (idx%4);
		  unsigned val = (rfp->bits[nibble] >> shift) & 3;
		  if (val & 1)
			char_val |= 1 << idx;
	    }

	    if (char_val != 0)
		  *cp++ = char_val;
      }

      for (unsigned idx = 0 ;  idx < nchar ;  idx += 1) {
	    unsigned bit = (nchar - idx - 1) * 8;
	    unsigned nibble = bit/4;
	    unsigned vall = rfp->bits[nibble+0];
	    unsigned valh = rfp->bits[nibble+1];

	    char char_val = 0;
	    if (vall&0x01) char_val |= 0x01;
	    if (vall&0x04) char_val |= 0x02;
	    if (vall&0x10) char_val |= 0x04;
	    if (vall&0x40) char_val |= 0x08;
	    if (valh&0x01) char_val |= 0x10;
	    if (valh&0x04) char_val |= 0x20;
	    if (valh&0x10) char_val |= 0x40;
	    if (valh&0x40) char_val |= 0x80;

	    if (char_val != 0)
		  *cp++ = char_val;
      }

      *cp = 0;
      vp->format = vpiStringVal;
      vp->value.str = buf;
}

static void binary_value(vpiHandle ref, p_vpi_value vp)
{
      struct __vpiBinaryConst*rfp = (struct __vpiBinaryConst*)ref;
      assert(ref->vpi_type->type_code == vpiConstant);

      switch (vp->format) {

	  case vpiObjTypeVal:
	  case vpiBinStrVal:
	    assert(rfp->nbits < sizeof buf);
	    for (unsigned idx = 0 ;  idx < rfp->nbits ;  idx += 1) {
		  unsigned nibble = idx/4;
		  unsigned shift  = 2 * (idx%4);
		  unsigned val = (rfp->bits[nibble] >> shift) & 3;

		  buf[rfp->nbits-idx-1] = "01xz"[val];
	    }
	    buf[rfp->nbits] = 0;
	    vp->value.str = buf;
	    vp->format = vpiBinStrVal;
	    break;

	  case vpiDecStrVal: {
		unsigned wid = rfp->nbits;
		unsigned char*tmp = new unsigned char[wid];
		for (unsigned idx = 0 ;  idx < wid ;  idx += 1)
		      tmp[idx] = (rfp->bits[idx/4] >> 2*(idx%4)) & 3;

		vpip_bits_to_dec_str(tmp, wid, buf, sizeof buf,
				     rfp->signed_flag);

		delete[]tmp;
		vp->value.str = buf;
		vp->format = vpiDecStrVal;
		break;
	  }

	  case vpiHexStrVal: {
		unsigned nchar = (rfp->nbits+3)/4;
		assert(nchar < sizeof buf);
		for (unsigned idx = 0 ;  idx < rfp->nbits ;  idx += 4) {
		      unsigned nibble = idx/4;
		      unsigned vals = rfp->bits[nibble];

		      if (vals == 0xff) {
			    buf[nchar-idx/4-1] = 'z';
		      } else if (vals == 0xaa) {
			    buf[nchar-idx/4-1] = 'x';
		      } else if (vals & 0xaa) {
			    buf[nchar-idx/4-1] = 'X';
		      } else {
			    unsigned val = vals&1;
			    if (vals&0x04) val |= 2;
			    if (vals&0x10) val |= 4;
			    if (vals&0x40) val |= 8;
			    buf[nchar-idx/4-1] = "0123456789abcdef"[val];
		      }
		}
		      
		buf[nchar] = 0;
		vp->value.str = buf;
		vp->format = vpiHexStrVal;
		break;
	  }

	  case vpiIntVal: {
		unsigned val = 0;
		unsigned bit_val = 0;
		unsigned bit_limit = rfp->nbits;
		if (bit_limit > 8*sizeof(val))
		      bit_limit = 8*sizeof(val);

		for (unsigned idx = 0 ;  idx < bit_limit ;  idx += 1) {
		      unsigned nibble = idx/4;
		      unsigned shift  = 2 * (idx%4);
		      bit_val = (rfp->bits[nibble] >> shift) & 3;
		      if (bit_val > 1) {
			    vp->value.integer = 0;
			    return;
		      } else {
			    val |= bit_val << idx;
		      }
		}

		/* sign extend */
		if (rfp->signed_flag && bit_val) {
		    for (unsigned idx = rfp->nbits; idx <sizeof(val)*8; idx++)
		    {
			val |= bit_val << idx;
		    }
		}

		vp->value.integer = val;
		break;
	  }

	  case vpiVectorVal: {
	      unsigned int obit = 0;
	      unsigned hwid = (rfp->nbits - 1)/32 + 1;
	      char*rbuf = need_result_buf(hwid*sizeof(s_vpi_vecval), RBUF_VAL);

	      s_vpi_vecval *op = (p_vpi_vecval)rbuf;
	      vp->value.vector = op;

	      op->aval = op->bval = 0;
	      for (unsigned idx = 0 ;  idx < rfp->nbits ;  idx += 1) {
		unsigned nibble = idx/4;
		unsigned shift  = 2 * (idx%4);
		unsigned bit_val = (rfp->bits[nibble] >> shift) & 3;

		switch (bit_val) {
		case 0:
		  op->aval &= ~(1 << obit);
		  op->bval &= ~(1 << obit);
		  break;
		case 1:
		  op->aval |= (1 << obit);
		  op->bval &= ~(1 << obit);
		  break;
		case 2:
		  op->aval |= (1 << obit);
		  op->bval |= (1 << obit);
		  break;
		case 3:
		  op->aval &= ~(1 << obit);
		  op->bval |= (1 << obit);
		  break;
		}
		obit++;
		if (!(obit % 32)) {
		      op += 1;
		      if ((op - vp->value.vector) < (long)hwid)
			    op->aval = op->bval = 0;
		      obit = 0;
		}
	      }
	      break;
	  }

	  case vpiStringVal:
	    binary_vpiStringVal(rfp, vp);
	    break;

	  default:
	    fprintf(stderr, "vvp error: format %d not supported "
		    "by vpiBinaryConst\n", vp->format);
	    vp->format = vpiSuppressVal;
	    break;
      }
}

static const struct __vpirt vpip_binary_rt = {
      vpiConstant,
      binary_get,
      0,
      binary_value,
      0,
      0,
      0
};

/*
 * Make a VPI constant from a vector string. The string is normally a
 * ASCII string, with each letter a 4-value bit. The first character
 * may be an 's' if the vector is signed.
 */
vpiHandle vpip_make_binary_const(unsigned wid, char*bits)
{
      struct __vpiBinaryConst*obj;

      obj = (struct __vpiBinaryConst*)
	    malloc(sizeof (struct __vpiBinaryConst));
      obj->base.vpi_type = &vpip_binary_rt;

      obj->signed_flag = 0;
      obj->nbits = wid;
      obj->bits = (unsigned char*)malloc((obj->nbits + 3) / 4);
      memset(obj->bits, 0, (obj->nbits + 3) / 4);

      const char*bp = bits;
      if (*bp == 's') {
	    bp += 1;
	    obj->signed_flag = 1;
      }

      for (unsigned idx = 0 ;  idx < obj->nbits ;  idx += 1) {
	    unsigned nibble = idx / 4;
	    unsigned val = 0;
	    switch (bp[wid-idx-1]) {
		case '0':
		  val = 0;
		  break;
		case '1':
		  val = 1;
		  break;
		case 'x':
		  val = 2;
		  break;
		case 'z':
		  val = 3;
		  break;
	    }

	    obj->bits[nibble] |= val << (2 * (idx%4));
      }

      free(bits);
      return &(obj->base);
}



static int dec_get(int code, vpiHandle ref)
{

      switch (code) {
	  case vpiConstType:
	    return vpiDecConst;

	  case vpiSigned: 
	    return 1;

	  case vpiSize:
	    return 32;

	  default:
	    fprintf(stderr, "vvp error: get %d not supported "
		    "by vpiDecConst\n", code);
	    assert(0);
	    return 0;
      }
}


static void dec_value(vpiHandle ref, p_vpi_value vp)
{
      struct __vpiDecConst*rfp = (struct __vpiDecConst*)ref;
      char* cp;
      assert(ref->vpi_type->type_code == vpiConstant);

      switch (vp->format) {

	  case vpiObjTypeVal:
	  case vpiIntVal: {
		vp->value.integer = rfp->value;
		break;
	  }

          case vpiDecStrVal:
	      sprintf(buf, "%d", rfp->value);

	      vp->format = vpiDecStrVal;
	      vp->value.str = buf;
	      break;

          case vpiBinStrVal:
	      cp = buf;
	      for(int bit=31; bit<=0;bit--){
		  *cp++ = "01"[ (rfp->value>>bit)&1 ];
	      }
	      *cp = 0;

	      vp->format = vpiBinStrVal;
	      vp->value.str = buf;
	      break;

          case vpiHexStrVal:
	      sprintf(buf, "%08x", rfp->value);

	      vp->format = vpiHexStrVal;
	      vp->value.str = buf;
	      break;

          case vpiOctStrVal:
	      sprintf(buf, "%011x", rfp->value);

	      vp->format = vpiOctStrVal;
	      vp->value.str = buf;
	      break;

	  default:
	    fprintf(stderr, "vvp error (vpi_const.cc): format %d not supported "
		    "by vpiDecConst\n", vp->format);
	    vp->format = vpiSuppressVal;
	    break;
      }
}

static const struct __vpirt vpip_dec_rt = {
      vpiConstant,
      dec_get,
      0,
      dec_value,
      0,
      0,
      0
};

vpiHandle vpip_make_dec_const(struct __vpiDecConst*obj, int value)
{
      obj->base.vpi_type = &vpip_dec_rt;
      obj->value = value;

      return &(obj->base);
}

vpiHandle vpip_make_dec_const(int value)
{
      struct __vpiDecConst*obj;

      obj = (struct __vpiDecConst*)
	    malloc(sizeof (struct __vpiDecConst));
      return vpip_make_dec_const(obj, value);
}


/*
 * $Log: vpi_const.cc,v $
 * Revision 1.24  2003/03/10 23:37:07  steve
 *  Direct support for string parameters.
 *
 * Revision 1.23  2003/03/10 19:14:27  steve
 *  More carful about shifting beyond word size.
 *
 * Revision 1.22  2003/02/24 06:35:45  steve
 *  Interactive task calls take string arguments.
 *
 * Revision 1.21  2002/11/03 20:33:43  steve
 *  Compiler error wrt ptrdiff_t.
 *
 * Revision 1.20  2002/11/03 02:07:24  steve
 *  Get VectorVals from constant values.
 *
 * Revision 1.19  2002/08/12 01:35:08  steve
 *  conditional ident string using autoconfig.
 *
 * Revision 1.18  2002/06/23 18:23:09  steve
 *  trivial performance boost.
 */

