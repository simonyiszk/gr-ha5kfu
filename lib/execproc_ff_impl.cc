/* -*- c++ -*- */
/* 
 * Copyright 2014 Andras Retzler <ha7ilm@sdr.hu>
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "execproc_ff_impl.h"
#include <gnuradio/gr_complex.h>
#include <stdio.h>
#include "popen2.h"
#include <fcntl.h>
#include <signal.h>

namespace gr {
  namespace ha5kfu {

    execproc_ff::sptr
    execproc_ff::make(std::string commandline)
    {
      return gnuradio::get_initial_sptr
        (new execproc_ff_impl(commandline));
    }

    /*
     * The private constructor
     */
    execproc_ff_impl::execproc_ff_impl(std::string commandline)
      : gr::block("execproc_ff",
              gr::io_signature::make(1, 1024, sizeof(float)),
              gr::io_signature::make(1, 1024, sizeof(float)))
    {
		this->commandline=commandline;
		child_pid=popen2(commandline.c_str(), &proc_stdin, &proc_stdout);
		if(!child_pid) fprintf(stderr, "[gr-ha5kfu/execproc] error: could not open child process!\n");
		else fprintf(stderr, "[gr-ha5kfu/execproc] child process opened!\n");
		int flags = fcntl(proc_stdout, F_GETFL, 0);
		fcntl(proc_stdout, F_SETFL, flags | O_NONBLOCK);
		flags = fcntl(proc_stdin, F_GETFL, 0);
		fcntl(proc_stdin, F_SETFL, flags | O_NONBLOCK);
	}

    /*
     * Our virtual destructor.
     */
    execproc_ff_impl::~execproc_ff_impl()
    {
    }

    bool
    execproc_ff_impl::start()
	{
		return true;
    }

    bool
    execproc_ff_impl::stop()
    {
		kill(child_pid, SIGKILL);
		return true;
    }

    void
    execproc_ff_impl::forecast (int noutput_items, gr_vector_int &ninput_items_required)
    {
        /* <+forecast+> e.g. ninput_items_required[0] = noutput_items */
    }

	#define EXECPROC_BUF_SIZE 1024

    int
    execproc_ff_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        // Do <+signal processing+>
        // Tell runtime system how many input items we consumed on
        // each input stream.
        const float *in = (const float *) input_items[0];
        float *out = (float *) output_items[0];
		
		if(!child_pid) return 0; //could not open process

        // Do signal processing...
		int result;
		int to_write=0;
		int in_remain=ninput_items[0];
		float* in_where=(float*)in;
		for(;;)
		{
			//fprintf(stderr, "to_write=%d in_remain=%d ninput_items[0]=%d\n", to_write, in_remain, ninput_items[0]);
			to_write=(in_remain>EXECPROC_BUF_SIZE)?EXECPROC_BUF_SIZE:in_remain;
			result=write(proc_stdin, in_where, to_write*sizeof(float));
			if(result<=0) break;	
			in_where+=result/sizeof(float);
			in_remain-=result/sizeof(float);
			if(result%sizeof(float)>0) fprintf(stderr, "[gr-ha5kfu/execproc] write atomicity error!\n");
			if(!in_remain) break;
		}
		consume_each (ninput_items[0]-in_remain);
		
		int out_remain=noutput_items;
		int to_read=0;
		float *read_where=out;
		for(;;)
		{
			//fprintf(stderr, "to_read=%d out_remain=%d noutput_items=%d\n", to_read, out_remain, noutput_items);
			to_read=(out_remain>EXECPROC_BUF_SIZE)?EXECPROC_BUF_SIZE:out_remain;
			result=read(proc_stdout, read_where, to_read*sizeof(float));
			if(result<=0) return noutput_items-out_remain;			
			read_where+=result/sizeof(float);
			out_remain-=result/sizeof(float);
			if(result%sizeof(float)>0) fprintf(stderr, "[gr-ha5kfu/execproc] read atomicity error!\n");
			if(!to_read) return noutput_items;
		}

    }

  } /* namespace ha5kfu */
} /* namespace gr */

