#pragma once
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#define eachlist(list,num) for(int num=0;num<list.Count;num++)
#define eachlists(list,start,num) for(int num=start;num<list.Count;num++)
void memcmp_ta(void * dst,const void * src , unsigned int size);
template<class ListItem>
class CTAList
{
	public:
	ListItem *Data;
public:
	CTAList(void)
	: ItemSize(0)
	, Count(0)
	, MaxCount(0)
	, Data(0)
	{
		ItemSize=sizeof(ListItem);
	}
	~CTAList(void)
	{
		Clear();
	}
	int ItemSize;
	int Count;
	int MaxCount;
	void Clear(void)
	{
		Count=0;
		MaxCount=0;
		if(Data) delete [] Data; Data=0;
	}

    void Reverse(int DefaultSize)
    {
        Count = 0;
        if ( MaxCount > DefaultSize )
        {
            Clear();
            ChangeMaxCount(DefaultSize);
        }
    }

    void Add(ListItem & Item)
    {
		Count=Count+1;
        if(Count>MaxCount)
        {
            ChangeMaxCount(MaxCount*2);
        }
		Data[Count-1]=Item;
    }

	void push_back(ListItem & Item)
	{
		Count=Count+1;
		if(Count>MaxCount) ChangeMaxCount(Count);
		Data[Count-1]=Item;
	}
	void push_back(ListItem & Item,int ChangeMax)
	{
		Count=Count+1;
		if(Count>MaxCount) ChangeMaxCount(Count+ChangeMax);
		Data[Count-1]=Item;
	}
	
	void ChangeMaxCount(int NewMaxCount)
	{
		if(NewMaxCount<=MaxCount) return;
		ListItem *ListDataSwap;

		ListDataSwap=new ListItem[NewMaxCount];
		if(Data)
		{
			memcmp_ta(ListDataSwap,Data,ItemSize*MaxCount);
			//memcpy_s(
			//	ListDataSwap,
			//	ItemSize*NewMaxCount,
			//	Data,
			//	ItemSize*MaxCount
			//	);
			delete [] Data;
		}
		Data=ListDataSwap;

		MaxCount=NewMaxCount;
	}
	ListItem & operator[](int Pos)
	{
		return Data[Pos];
	}
	void operator=(CTAList & DataIn)
	{
		if(DataIn.Count==0)
		{
			Count=0;
			return;
		}
		bool Change=(MaxCount!=DataIn.MaxCount)||(ItemSize!=DataIn.ItemSize);
		if(Change)
			Clear();
		ItemSize=DataIn.ItemSize;
		Count=DataIn.Count;
		MaxCount=DataIn.MaxCount;
		if(Data==0)
			Data=new ListItem[MaxCount];
			memcmp_ta(Data,DataIn.Data,ItemSize*MaxCount);
		//memcpy_s(
		//		Data,
		//		ItemSize*Count,
		//		DataIn.Data,
		//		ItemSize*Count
		//		);
	}
	ListItem & Back()
	{
		return Data[Count-1];
	}
	void DelBack()
	{
		Count=max(0,Count-1);
	}
	void operator+=(CTAList<ListItem> & DataIn)
	{
		ChangeMaxCount(Count+DataIn.Count);
		memcmp_ta(Data+Count,DataIn.Data,ItemSize*DataIn.Count);
		Count+=DataIn.Count;
	}
	void operator+=(ListItem & DataIn)
	{
		Count=Count+1;
		if(Count>MaxCount) ChangeMaxCount(Count);
		Data[Count-1]=DataIn;
	}
};