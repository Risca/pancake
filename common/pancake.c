
//_____ I N C L U D E S________________________________________________________
#include <pancake.h>
#include <stddef.h>
#include <string.h>

#if PANC_HELPERS_ENABLED != 0
#include <helpers.h>
#endif

#if PANC_HAVE_PRINTF != 0
#include <stdio.h>
#endif

#ifdef _WIN32
    #include <windows.h>
    #include <winsock.h>
#endif



//_____ D E F I N I T I O N S _________________________________________________
/* Updated to reflect RFC6282 */
#define DISPATCH_NALP        0x00
#define DISPATCH_ESC         0x40
#define DISPATCH_IPv6        0x41
#define DISPATCH_HC1         0x42
#define DISPATCH_BC0         0x50
#define DISPATCH_IPHC        0x7F
#define DISPATCH_MESH        0x80
#define DISPATCH_FRAG1       0xC0
#define DISPATCH_FRAGN       0xE0

/* None, 32 bit, 64 bit, 128 bit */
const uint16_t security_overhead[] = 
	{0, 9, 13, 21};

struct pancake_frag_hdr {
	uint16_t size;
	uint16_t tag;
	uint8_t offset;
};

struct pancake_main_dev {
	struct pancake_port_cfg		*cfg;
	struct pancake_options_cfg	*options;
	void						*dev_data;
	event_callback_func			event_callback;
};
static struct pancake_main_dev devs[PANC_MAX_DEVICES];

//static struct pancake_event_data_received data_received;
static pancake_event event;

#if PANC_HAVE_PRINTF == 0
	#define pancake_printf(...) {}
#else
	#define pancake_printf(...) printf(__VA_ARGS__)
#endif

#if PANC_HELPERS_ENABLED == 0
	#define pancake_print_raw_bits(...) {}
#else
	#define pancake_print_raw_bits(...) pancake_print_raw_bits(__VA_ARGS__)
	extern void pancake_print_raw_bits(FILE *out, uint8_t *bytes, size_t length);
#endif


static PANCSTATUS pancake_send_fragmented(struct pancake_main_dev *dev, uint8_t *raw_data, struct pancake_compressed_ip6_hdr *comp_hdr, uint8_t *payload, uint16_t payload_len);


//_____ F U N C T I O N   D E F I N I T I O N S________________________________
static void print_pancake_error(char *source, PANCSTATUS ret)
{
	switch (ret) {
	case PANCSTATUS_ERR:
		pancake_printf("%s: Undefined error\n", source);
		break;
	case PANCSTATUS_NOMEM:
		pancake_printf("%s: Not enough memory!\n", source);
		break;
	case PANCSTATUS_NOTREADY:
		pancake_printf("%s: Not ready\n", source);
		break;
	default:
		break;
	}
}

static uint16_t calculate_frame_overhead(struct pancake_main_dev *dev, struct pancake_compressed_ip6_hdr *hdr)
{
	uint16_t overhead = 0;
	struct pancake_options_cfg *options = dev->options;

	/* Overhead due to security */
	overhead += security_overhead[options->security];

	/* Compression overhead (+1 dispatch byte) */
	overhead += hdr->size + 1;

	return overhead;
}


#include "pancake_internals/fragmentation.c"
#include "pancake_internals/reassembly.c"
#include "pancake_internals/header_compression.c"
#include "pancake_internals/address_autoconfiguration.c"


PANCSTATUS pancake_init(PANCHANDLE *handle, struct pancake_options_cfg *options_cfg, struct pancake_port_cfg *port_cfg, void *dev_data, event_callback_func event_callback)
{
	int8_t ret;
	uint8_t i;
	static uint8_t handle_count = 0;
	struct pancake_main_dev *dev = &devs[handle_count];
	
	/* Sanity check */
	if (handle == NULL || options_cfg == NULL || port_cfg == NULL || handle_count+1 > PANC_MAX_DEVICES) {
		goto err_out;
	}
	if (port_cfg->write_func == NULL) {
		goto err_out;
	}

	/* Mark reassembly buffers as free */
	for (i = 0; i < PANC_MAX_CONCURRENT_REASSEMBLIES; i++) {
		ra_bufs[i].free = 1;
	}

	/* Initialize port */
	if (port_cfg->init_func) {
		ret = port_cfg->init_func(dev_data);
		if (ret != PANCSTATUS_OK) {
			goto err_out;
		}
	}

	/* Save config and device data, and update handle */
	dev->cfg = port_cfg;
	dev->options = options_cfg;
	dev->dev_data = dev_data;
	dev->event_callback = event_callback;
	*handle = handle_count;
	handle_count++;

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

#if PANC_TESTS_ENABLED != 0

PANCSTATUS pancake_write_test(PANCHANDLE handle)
{
	uint8_t ret;
	struct pancake_main_dev	*dev = &devs[handle];
	struct ip6_hdr hdr = {
		.ip6_flow	=	htonl(6 << 28),
		.ip6_plen	=	htons(255),
		.ip6_nxt	=	254,
		.ip6_hops	=	255,
		.ip6_src	=	{
			/* Loopback (::1/128) */
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0xff, 0xfe, 0, 0, 1,
		},
		.ip6_dst	=	{
			/* Loopback (::1/128) */
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 1,
		},
	};
	uint16_t length	= sizeof(hdr);

	ret = dev->cfg->write_func(dev->dev_data, NULL, (uint8_t*)&hdr, length);
	if (ret != PANCSTATUS_OK || length != sizeof(hdr)) {
		goto err_out;
	}

	//dev->event_callback(NULL, "Write test successful!", 0);

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
#endif

void pancake_destroy(PANCHANDLE handle)
{
	struct pancake_main_dev *dev = &devs[handle];
	PANCSTATUS ret;

	ret = dev->cfg->destroy_func(dev->dev_data);
	if (ret != PANCSTATUS_OK) {
		/* What to do, what to do? */
	}
}

PANCSTATUS pancake_send(PANCHANDLE handle, struct ip6_hdr *hdr, uint8_t *payload, uint16_t payload_length)
{
	uint8_t raw_data[aMaxPHYPacketSize - aMaxFrameOverhead];
	uint8_t hdr_buff[40];
	uint16_t length;
	struct pancake_main_dev *dev;
	struct pancake_compressed_ip6_hdr compressed_ip6_hdr;
	compressed_ip6_hdr.hdr_data = hdr_buff;
	uint16_t frame_overhead;
	PANCSTATUS ret;
	memset( raw_data, 0, aMaxPHYPacketSize - aMaxFrameOverhead );
	struct pancake_ieee_addr destination_address;
	
#if PANC_USE_COLOR != 0
	struct color_change color_positions[3];
#endif

	/* Sanity check */
	if ( 	handle < 0 ||
			handle > PANC_MAX_DEVICES ||
			hdr == NULL ||
			(payload == NULL && payload_length != 0) ) {
		goto err_out;
	}
	dev = &devs[handle];

	switch (dev->options->compression) {
	case PANC_COMPRESSION_NONE:
		compressed_ip6_hdr.dispatch_value = DISPATCH_IPv6;
		compressed_ip6_hdr.hdr_data = (uint8_t *)hdr;
		compressed_ip6_hdr.size = 40;
		break;
	case PANC_COMPRESSION_IPHC:
		ret = pancake_compress_header(hdr, &compressed_ip6_hdr);
		if(ret != PANCSTATUS_OK) {
			printf("%s", "There is an error in pancake_compress_header");
			goto err_out;
		}

		compressed_ip6_hdr.dispatch_value = DISPATCH_IPHC;
		break;
	default:
		/* Not supported... yet */
		goto err_out;
	}

	/* Check if fragmentation is needed */
	frame_overhead = calculate_frame_overhead(dev, &compressed_ip6_hdr);
	if (frame_overhead + aMaxFrameOverhead + payload_length > aMaxPHYPacketSize) {
		
		/* pancake_send_fragmented() sends the whole payload, but in multiple packets */
		ret = pancake_send_fragmented(dev, raw_data, &compressed_ip6_hdr, payload, payload_length);

		if (ret != PANCSTATUS_OK) {
			goto err_out;
		}
	}
	/* Packet fits inside a single packet, copy header and payload to raw_data and send */
	else {
		/* Dispatch value */
		*raw_data = compressed_ip6_hdr.dispatch_value;
		/* Copy hdr and payload */
		memcpy((uint8_t*)(raw_data + 1), (uint8_t*)compressed_ip6_hdr.hdr_data, compressed_ip6_hdr.size);
		memcpy((uint8_t*)(raw_data + 1 + compressed_ip6_hdr.size), (uint8_t*)payload, payload_length);

		length = frame_overhead+payload_length;

#if PANC_USE_COLOR != 0
		/* Print packet */
		color_positions[0].color = PANC_COLOR_RED;
		color_positions[0].position = 1;
		color_positions[0].description = "6LoWPAN header";
		
		color_positions[1].color = PANC_COLOR_GREEN;
		color_positions[1].position = 1 + compressed_ip6_hdr.size;
		color_positions[1].description = "IPv6 header";
		
		color_positions[2].color = PANC_COLOR_BLUE;
		color_positions[2].position = length;
		color_positions[2].description = "Payload";
		pancake_pretty_print(dev->dev_data, raw_data, length, color_positions, 3);
#endif
		// Get address
		ret = pancake_get_ieee_address_from_ipv6(&destination_address, &hdr->ip6_dst);
		if(ret != PANCSTATUS_OK) {
			goto err_out;
		}
		
		ret = dev->cfg->write_func(dev->dev_data, &destination_address, raw_data, length);
		if (ret != PANCSTATUS_OK) {
			goto err_out;
		}
	}

	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}

PANCSTATUS pancake_send_packet(PANCHANDLE handle, uint8_t *ip6_packet, uint16_t packet_length)
{
	return pancake_send(handle, (struct ip6_hdr *)ip6_packet, ip6_packet+40, packet_length-40);
}

PANCHANDLE pancake_handle_from_dev_data(void *dev_data)
{
	int i;
	struct pancake_main_dev *dev;
	for (i=0; i<PANC_MAX_DEVICES; i++) {
		dev = &devs[i];
		if (dev->dev_data == dev_data) {
			return i;
		}
	}
	return -1;
}

PANCSTATUS pancake_process_data(void *dev_data, struct pancake_ieee_addr *src, struct pancake_ieee_addr *dst, uint8_t *data, uint16_t size)
{
	uint8_t *payload;
	uint16_t payload_length;
	struct ip6_hdr *hdr;
	PANCSTATUS ret = PANCSTATUS_ERR;
	PANCHANDLE handle;
	struct pancake_main_dev *dev;
	struct pancake_reassembly_buffer *ra_buf = NULL;
	struct pancake_compressed_ip6_hdr compressed_ip6_hdr;

	/* Try to get handle */
	handle = pancake_handle_from_dev_data(dev_data);
	if (handle < 0) {
		goto out;
	}
	dev = &devs[handle];

	while( PANCSTATUS_OK != ret ) {
	  
		/* Read dispatch value */
		switch (*data) {
		case DISPATCH_IPv6:
		  	/* If NOT fragmented, set length as size - header - dispatch */
			if ( NULL == ra_buf ) {
				payload_length = size - (1 + 40); 
			}
			/* If fragmented, get length from reassembly - header - dispatch */
			else {
				payload_length = ra_buf->octets_received - (1 + 40);
			}
		  
			hdr = (struct ip6_hdr*)(data + 1);
			payload = data + 1 + 40;
			ret = PANCSTATUS_OK;
			break;
		case DISPATCH_HC1:
		case DISPATCH_BC0:
		case DISPATCH_IPHC:
		    
		  	/* TODO: 	Since we do not know how much the header in compressed we do
		  				not know the size of it (?). Thus we cannot calculate the
		   				location, or size, of the payload. payload_length should be
		  				removed from pancake_decompress_header. */

			// Payload_length from 802.15.4 packet or fragmentation header 
			compressed_ip6_hdr.hdr_data = (data+1);
			ret = pancake_decompress_header(&compressed_ip6_hdr, hdr, payload_length);
			if (PANCSTATUS_OK != ret ) {
			    goto out;
			}
			
			/* If NOT fragmented, set length as size - header - dispatch */
			if ( NULL == ra_buf ) {
				payload_length = size - (1 + compressed_ip6_hdr.size); 
			}
			/* If fragmented, get length from reassembly - header - dispatch */
			else {
				payload_length = ra_buf->octets_received - (1 + compressed_ip6_hdr.size);
			}
			
			/* This should probably be set here since it will depend on if it was fragmented */
			hdr->ip6_plen = htons(payload_length);
			
			payload = data + 1 + compressed_ip6_hdr.size; //????
			ret = PANCSTATUS_OK;
			break;
		default:
			switch (*data & 0xF8) { // 0b11111000
			case DISPATCH_FRAG1:
			case DISPATCH_FRAGN:				
				ret = pancake_reassemble(dev, &ra_buf, src, dst, data, size);
				if (ret != PANCSTATUS_OK) {
					goto out;
				}
				
				data = ra_buf->data;
				ret = PANCSTATUS_NOTREADY;
				break;
			default:
				if ((*data & 0xC0) == DISPATCH_MESH) {
					/* Not implemented yet */
					goto out;
				}
				else {
					/* Not a LowPAN frame */
					pancake_printf("Not a LoWPAN frame?:\n");
					pancake_print_raw_bits(NULL, data, size);
					goto out;
				}
			}
		};
	}
	// TODO: AT THIS POINT THE IPv6 HEADER, hdr, SHOULD BE DECOMPRESSED(?)
	
	event.type = PANC_EVENT_DATA_RECEIVED;
	event.data_received.hdr = hdr;
	event.data_received.payload = payload;
	event.data_received.payload_length = payload_length;
	
	/* Relay data to upper levels */
	dev->event_callback(&event);

out:
	print_pancake_error("pancake_process_data()", ret);
	return ret;
}

PANCSTATUS pancake_connection_update(void *dev_data, struct pancake_event_con_update *connection_update )
{
  	/* Try to get handle */
  	struct pancake_main_dev *dev;
	PANCHANDLE handle;
	handle = pancake_handle_from_dev_data(dev_data);
	if (handle < 0) {
		goto err_out;
	}
	dev = &devs[handle];
  
  	event.type = PANC_EVENT_CONNECTION_UPDATE;
	event.connection_update = *connection_update;
	
	dev->event_callback(&event);
	
	return PANCSTATUS_OK;
err_out:
	return PANCSTATUS_ERR;
}
