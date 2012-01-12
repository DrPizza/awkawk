#pragma once

#ifndef SHARED_TEXTURE_QUEUE__H
#define SHARED_TEXTURE_QUEUE__H

#include "stdafx.h"

#include "util.h"

#include "utility/interlocked_containers.hpp"

static const GUID shared_handle_guid = { 0xfad31a05, 0x1099, 0x4eb5, 0x9e, 0x30, 0x71, 0x78, 0x18, 0xfa, 0xf1, 0xe2 };

struct shared_texture_queue : boost::noncopyable {
	struct shared_texture_queue_desc {
		D3DFORMAT format;
		DWORD width;
		DWORD height;
		DWORD minimum_textures;
	};

	shared_texture_queue() {
	}

	explicit shared_texture_queue(const shared_texture_queue_desc& description_) : description(new shared_texture_queue_desc(description_)) {
	}

	void set_parameters(const shared_texture_queue_desc& description_) {
		description.reset(new shared_texture_queue_desc(description_));
		reset();
	}

	void set_producer(IDirect3DDevice9Ptr producer_) {
		producer = producer_;
		reset();
	}

	void set_consumer(IDirect3DDevice9Ptr consumer_) {
		consumer = consumer_;
		reset();
	}

	void producer_enqueue(IDirect3DTexture9Ptr txtr) {
		produced->push(get_handle(txtr));
	}

	IDirect3DTexture9Ptr producer_dequeue() {
		std::pair<bool, HANDLE> next_handle(consumed->pop());
		while(!next_handle.first) {
			generate_texture_pair();
			next_handle = consumed->pop();
		}
		std::pair<bool, IDirect3DTexture9Ptr> next_texture(producer_lookup->find(next_handle.second));
		return next_texture.second;
	}

	IDirect3DTexture9Ptr consumer_dequeue() {
		std::pair<bool, HANDLE> next_handle(produced->pop());
		if(!next_handle.first) {
			return nullptr;
		}
		std::pair<bool, IDirect3DTexture9Ptr> next_texture(consumer_lookup->find(next_handle.second));
		return next_texture.second;
	}

	void consumer_enqueue(IDirect3DTexture9Ptr txtr) {
		consumed->push(get_handle(txtr));
	}

	bool backlogged() const {
		return produced && !produced->empty();
	}

private:
	void set_handle(IDirect3DTexture9Ptr txtr, HANDLE h) {
		txtr->SetPrivateData(shared_handle_guid, &h, sizeof(h), 0);
	}

	HANDLE get_handle(IDirect3DTexture9Ptr txtr) {
		HANDLE h;
		DWORD size(sizeof(h));
		txtr->GetPrivateData(shared_handle_guid, &h, &size);
		return h;
	}

	void generate_texture_pair() {
		IDirect3DTexture9Ptr txtr;
		HANDLE h(NULL);
		FAIL_THROW(producer->CreateTexture(description->width, description->height, 1, D3DUSAGE_DYNAMIC, description->format, D3DPOOL_DEFAULT, &txtr, &h));
		txtr->SetPrivateData(shared_handle_guid, &h, sizeof(h), 0);
		producer_lookup->insert(h, txtr);

		FAIL_THROW(consumer->CreateTexture(description->width, description->height, 1, D3DUSAGE_DYNAMIC, description->format, D3DPOOL_DEFAULT, &txtr, &h));
		txtr->SetPrivateData(shared_handle_guid, &h, sizeof(h), 0);
		consumer_lookup->insert(h, txtr);

		consumed->push(h);
	}

	void reset() {
		produced.reset(new utility::interlocked_queue<HANDLE>());
		consumed.reset(new utility::interlocked_queue<HANDLE>());
		producer_lookup.reset(new utility::interlocked_kv_list<HANDLE, IDirect3DTexture9Ptr>());
		consumer_lookup.reset(new utility::interlocked_kv_list<HANDLE, IDirect3DTexture9Ptr>());

		if(producer && consumer && description) {
			for(DWORD i(0); i < description->minimum_textures; ++i) {
				generate_texture_pair();
			}
		}
	}

	IDirect3DDevice9Ptr producer;
	IDirect3DDevice9Ptr consumer;

	std::shared_ptr<utility::interlocked_queue<HANDLE> > produced;
	std::shared_ptr<utility::interlocked_kv_list<HANDLE, IDirect3DTexture9Ptr> > producer_lookup;
	std::shared_ptr<utility::interlocked_queue<HANDLE> > consumed;
	std::shared_ptr<utility::interlocked_kv_list<HANDLE, IDirect3DTexture9Ptr> > consumer_lookup;

	std::unique_ptr<shared_texture_queue_desc> description;
};

#endif
