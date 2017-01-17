#include "B4RDefines.h"
namespace B4R {
	
	
	//static
	void AsyncStreams::checkForData(void* b) {
		AsyncStreams* me = (AsyncStreams*) b;
		if (me->stream->wrappedClient != NULL) {
			if (me->stream->wrappedClient->connected() == false) {
				me->Close();
				if (me->ErrorSub != NULL)
					me->ErrorSub();
				return;
			}
			#ifdef ESP_H
			me->stream->wrappedClient->flush();
			#endif

		}
		
		int av = me->stream->BytesAvailable();
		if (av == 0)
			return;
		const UInt cp = B4R::StackMemory::cp;
		ArrayByte* arr = CreateStackMemoryObject(ArrayByte);
		arr->data = StackMemory::buffer + StackMemory::cp;
		int index = 0;
		while (index < me->MaxBufferSize) {
			if (av) {
				int count = me->stream->ReadBytes(arr, index, Common_Min(av, me->MaxBufferSize - index));
				index += count;
			}
			else {
				break;
			}
			av = me->stream->BytesAvailable();
			if (!av) {
				if (me->WaitForMoreDataDelay)
					delay(me->WaitForMoreDataDelay);
				av = me->stream->BytesAvailable();
			}
		}
		if (index > 0) {
			arr->length = index;
			sender->wrapPointer(b);
			B4R::StackMemory::cp += index;
			me->NewDataSub(arr);
		}
		B4R::StackMemory::cp = cp;
	}
	void AsyncStreams::Initialize (B4RStream* Stream, SubVoidArray NewDataSub, SubVoidVoid ErrorSub) {
		this->NewDataSub = NewDataSub;
		this->ErrorSub = ErrorSub;
		FunctionUnion fu;
		fu.PollerFunction = checkForData;
		pnode.functionUnion = fu;
		pnode.tag = this;
		if (pnode.next == NULL) {
			pollers.add(&pnode);
		}
		this->stream = Stream;
	}
	AsyncStreams* AsyncStreams::Write (ArrayByte* Data) {
		return Write2(Data, 0, Data->length);
	}
	AsyncStreams* AsyncStreams::Write2 (ArrayByte* Data, UInt Start, UInt Length) {
		UInt i = stream->WriteBytes (Data, Start, Length);
		if (i < Length) {
			sender->wrapPointer(this);
			Close();
			if (this->ErrorSub != NULL)
				this->ErrorSub();
			
		}
		return this;
	}
	
	void AsyncStreams::Close() {
		if (pnode.next != NULL)
			pollers.remove(&pnode);
	}
}