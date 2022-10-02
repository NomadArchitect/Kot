#include <multiboot/pmm/pmm.h>

uint32_t Pmm_MbHeader;
uint32_t Pmm_SizeMbHeader;
multiboot_tag_mmap* Pmm_Map;
uint64_t Pmm_LastIndexAllocated;
uint64_t Pmm_LastAddressAllocated;
uint64_t Pmm_MapNumberEntry;
uint64_t Pmm_PageNumber;
uint64_t Pmm_Size;

uint32_t __attribute__((section(".start"))) Pmm_Ukl_Start;
uint32_t __attribute__((section(".end"))) Pmm_Ukl_End;

void Pmm_Init(uint32_t MbHeader, multiboot_tag_mmap* Map){
    Pmm_Map = Map;
    Pmm_MbHeader = MbHeader;
    Pmm_SizeMbHeader = *(uint32_t*)MbHeader; // get size

    Pmm_MapNumberEntry = (Pmm_Map->size - sizeof(multiboot_tag_mmap)) / Pmm_Map->entry_size;
    for(uint64_t i = 0; i < Pmm_MapNumberEntry ; i++){
        multiboot_mmap_entry* entry = &Pmm_Map->entries[i];
        if(entry->type == MULTIBOOT_MEMORY_AVAILABLE){
            Pmm_LastIndexAllocated = i;
            Pmm_LastAddressAllocated = entry->addr;
            break;
        }
    }

    if(Pmm_LastAddressAllocated < FIRST_USABLE_MEMORY){
        Pmm_LastAddressAllocated = FIRST_USABLE_MEMORY;
    }

    Pmm_Size = Pmm_GetMemorySize(Map);
    Pmm_PageNumber = (Pmm_Size >> 12) + 1;
}

uint64_t Pmm_GetMemorySize(multiboot_tag_mmap* Map){
    static uint64_t memorySizeBytes = 0;
    if (memorySizeBytes > 0) return memorySizeBytes;

    for(uint64_t i = 0; i < Map->entry_size; i++){
        memorySizeBytes += Map->entries[i].len;
    }

    return memorySizeBytes;
}

bool Pmm_CheckPage(uint64_t Address){
    if(Address + PAGE_SIZE > Pmm_MbHeader && Address < Pmm_MbHeader + Pmm_SizeMbHeader) return false;
    if(Address + PAGE_SIZE > (uint64_t)&Pmm_Ukl_Start && Address < (uint64_t)&Pmm_Ukl_End) return false;
    for(struct multiboot_tag* tag = (struct multiboot_tag*)(Pmm_MbHeader + 8); tag->type != MULTIBOOT_TAG_TYPE_END; tag = (struct multiboot_tag*) ((multiboot_uint8_t*) tag + ((tag->size + 7) & ~7))){
        switch (tag->type){
            case MULTIBOOT_TAG_TYPE_MODULE:
                if(Address + PAGE_SIZE > ((struct multiboot_tag_module*)tag)->mod_start && Address < ((struct multiboot_tag_module*)tag)->mod_end) return false;
                break;
            case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:{
                uint64_t fb_size = ((struct multiboot_tag_framebuffer_common*)tag)->framebuffer_pitch * ((struct multiboot_tag_framebuffer_common*)tag)->framebuffer_height;
                if(Address + PAGE_SIZE > ((struct multiboot_tag_framebuffer_common*)tag)->framebuffer_addr && Address < ((struct multiboot_tag_framebuffer_common*)tag)->framebuffer_addr + fb_size) return false;
                break;
            }
            default:
                break;
        }
    } 
    return true;  
}

uint64_t Pmm_RequestPage(){
    for(uint64_t i = Pmm_LastIndexAllocated; i < Pmm_MapNumberEntry; i++){
        multiboot_mmap_entry* entry = &Pmm_Map->entries[i];
        if(entry->type == MULTIBOOT_MEMORY_AVAILABLE){
            for(uint64_t y = 0; y < entry->len; y += PAGE_SIZE){
                uint64_t AddressAllocated = Pmm_LastAddressAllocated;
                Pmm_LastAddressAllocated += PAGE_SIZE;
                if(Pmm_CheckPage(Pmm_LastAddressAllocated)){
                    size64_t SizeAvailable = (entry->addr + entry->len) - AddressAllocated;
                    if(SizeAvailable < PAGE_SIZE){
                        Pmm_LastIndexAllocated++; // go to the next segment
                        Pmm_LastAddressAllocated = Pmm_Map->entries[i + 1].addr;
                        break;
                    }else{
                        return (uint64_t)AddressAllocated;
                    }
                }
            }
        }
    }
    return NULL; 
}

void Pmm_AddEntry(struct ukl_mmap_info_t** entry, uint64_t* index, uint64_t base, size64_t length, uint64_t type){
    struct ukl_mmap_info_t* entries = (*entry);
    entries[*index].base = base;
    entries[*index].length = length;
    entries[*index].type = type;
    *index++;

    if(*index > MAX_ENTRY_IN_PAGE_UKL){
        *entry = (struct ukl_mmap_info_t*)Pmm_RequestPage();
        entries[*index - 1].map_next_entry = *entry;
        *index = 0;
    }else{
        entries[*index - 1].map_next_entry = &entries[*index];
    }
}

void Pmm_ExtractsInfo(struct ukl_boot_structure_t* BootData){
    BootData->memory_info.page_count_total = Pmm_PageNumber;

    struct ukl_mmap_info_t* MainEntry = (struct ukl_mmap_info_t*)Pmm_RequestPage();
    uint64_t EntryIndex = 0;
    BootData->memory_info.map_main_entry = MainEntry;
    for(uint64_t i = 0; i < Pmm_MapNumberEntry; i++){
        multiboot_mmap_entry* entry = &Pmm_Map->entries[i];
        if(entry->type == MULTIBOOT_MEMORY_AVAILABLE){
            if(i > Pmm_LastIndexAllocated){
                Pmm_AddEntry(&MainEntry, &EntryIndex, entry->addr, entry->len, UKL_MMAP_AVAILABLE);
            }else if(i < Pmm_LastIndexAllocated){
                Pmm_AddEntry(&MainEntry, &EntryIndex, entry->addr, entry->len, UKL_MMAP_USED);
            }else{
                uint64_t LowerHalfSegmentLength = Pmm_LastAddressAllocated - entry->addr;
                uint64_t HigherHalfSegmentLength = entry->len - LowerHalfSegmentLength;

                uint64_t LowerHalfSegmentAddress = entry->addr;
                uint64_t HigherHalfSegmentAddress = Pmm_LastAddressAllocated;
                Pmm_AddEntry(&MainEntry, &EntryIndex, LowerHalfSegmentAddress, LowerHalfSegmentLength, UKL_MMAP_USED);
                Pmm_AddEntry(&MainEntry, &EntryIndex, HigherHalfSegmentAddress, HigherHalfSegmentLength, UKL_MMAP_AVAILABLE);
            }
        }else{
            Pmm_AddEntry(&MainEntry, &EntryIndex, entry->addr, entry->len, UKL_MMAP_RESERVED);
        }
    }
}