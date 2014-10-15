#ifndef CLASS_IRQ_HANDLER_H
#define CLASS_IRQ_HANDLER_H

#include <class_os.hpp>
#include "irq/pic_defs.h"

#include <delegate>
#include <hw/pic.h>

/*
  IDT Type flags  
*/

//Bits 0-3, "type" (VALUES)
const char TRAP_GATE=0xf;
const char TASK_GATE=0x5;
const char INT_GATE=0xe;


//Bit 4, "Storage segment" (BIT NUMBER)
const char BIT_STOR_SEG=0x10; //=0 for trap gates

//Bits 5-6, "Protection level" (BIT NUMBER)
const char BIT_DPL1=0x20;
const char BIT_DPL2=0x40;

//Bit 7, "Present" (BIT NUMBER)
const char BIT_PRESENT=0x80;


//From osdev
struct IDTDescr{
  uint16_t offset_1; // offset bits 0..15
  uint16_t selector; // a code segment selector in GDT or LDT
  uint8_t zero;      // unused, set to 0
  uint8_t type_attr; // type and attributes, see below
  uint16_t offset_2; // offset bits 16..31
};

struct idt_loc{
  uint16_t limit;
  uint32_t base;
}__attribute__ ((packed));

/** We'll limit the number of subscribable IRQ lines to this.  */
typedef uint32_t irq_bitfield;


//irq_bitfield irq_pending;

extern "C" {
  void irq_default_handler();
}



/** A class to handle interrupts. 
  
    Anyone can subscribe to IRQ's, but the will be indireclty called via the
    Deferred Procedure Call (DPC) system, i.e. when the system is in a 
    wait-loop with nothing else to do.
 */
class IRQ_handler{

 public:
  
  typedef delegate<void()> irq_delegate;
  
  /** Enable an IRQ line.  If no handler is set a default will be used */
  static void enable_irq(uint8_t irq);
  
  /** Directly set an IRQ handler in IDT.       
      @param irq : The IRQ to handle
      @param function_addr : A proper IRQ handler
      @warning{ 
      This has to be a function that properly returns with `iret`.
      Failure to do so will keep the interrupt from firing and cause a 
      stack overflow or similar badness.
      }
  */
  static void set_handler(uint8_t irq, void(*function_addr)());
    
  /** Subscribe to an IRQ. 
      
      @param del a delegate to attach to the IRQ DPC-system, 
      
      the delagete will be called a.s.a.p. after @param irq gets triggered.
      @warning The delegate is responsible for signalling a proper EOI.
      @todo Implies enable_irq(irq)? 
      @todo Create a public member IRQ_handler::eoi for delegates to use
  */
  static void subscribe(uint8_t irq, irq_delegate del);
  
  static void subscribe(uint8_t irq, void(*notify)());
 

private:
  static unsigned int irq_mask;
  static int timer_interrupts;
  static IDTDescr idt[256];
  static const char default_attr=0x8e;
  static const uint16_t default_sel=0x8;
  static bool idt_is_set;
  
  /** bit n set means IRQ n has fired since last check */
  //static irq_bitfield irq_pending;
  static irq_bitfield irq_subscriptions;
  
  static void(*irq_subscribers[sizeof(irq_bitfield)*8])();
  static irq_delegate irq_delegates[sizeof(irq_bitfield)*8];
  
  /** STI */
  static void enable_interrupts();
  
  /** @deprecated A default handler */
  static void handle_IRQ_default();
  
  /** Create an IDT-gate. Use "set_handler" for a simpler version 
      using defaults */
  static void create_gate(IDTDescr* idt_entry,
			  void (*function_addr)(),
			  uint16_t segment_sel,
			  char attributes
			  );
  
  /** The OS will call the following : */
  friend class OS;
  friend void ::irq_default_handler();
  
  /** Initialize. Only the OS can initialize the IRQ manager */
  static void init();
  
  /** Notify all delegates waiting for interrupts */
  static void notify();
  
  static void eoi(uint8_t irq);
  
};


#endif