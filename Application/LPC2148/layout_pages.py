"""Normalize legacy HTML layout without changing form names or stored values."""
from lxml import etree, html


def normalize_layout(body, number):
    root = html.fragment_fromstring(body, create_parent='div')
    for node in list(root.xpath('.//p[not(*)]')):
        if not (node.text or '').strip():
            node.drop_tree()

    # Classic menus lost their external stylesheet in the original import.
    # Native details restore compact, keyboard-accessible dropdowns offline.
    for item in root.xpath('.//div[contains(concat(" ",normalize-space(@class)," ")," menu ")]/ul/li'):
        submenu = item.find('ul')
        anchor = item.find('a')
        if submenu is not None and anchor is not None:
            details = etree.Element('details')
            summary = etree.SubElement(details, 'summary')
            summary.text = ' '.join(anchor.itertext()).replace('▼', '').strip()
            item.remove(anchor)
            item.insert(0, details)
            details.append(submenu)
    for cell in root.xpath('.//td'):
        text = ''.join(cell.itertext()).strip()
        if text and set(text) <= set('. \n\t\r\u00a0') and all(n.tag == 'br' for n in cell):
            cell.text = None
            for n in list(cell):
                cell.remove(n)
    for br in list(root.xpath('.//br')):
        previous = br.getprevious()
        if previous is not None and previous.tag == 'br' and not (previous.tail or '').strip():
            br.drop_tree()

    # Several original templates never close their outer tile UL. Restore
    # sibling cards before applying a grid, keeping each card in its own form.
    cards = root.xpath('.//ul[contains(@class,"ui-block-")]')
    card_set = set(cards)
    for card in cards:
        ancestors = [a for a in card.iterancestors() if a in card_set]
        if ancestors:
            outer = ancestors[-1]
            owner = next((a for a in card.iterancestors() if a.tag == 'form'), None)
            outer_owner = next((a for a in outer.iterancestors() if a.tag == 'form'), None)
            if owner is outer_owner:
                outer.addnext(card)
    # addnext reverses subsequent siblings when their ancestor stays the same;
    # sort groups using the original document order captured above.
    order = {card: index for index, card in enumerate(cards)}
    for parent in {c.getparent() for c in cards}:
        group = [c for c in parent if c in card_set]
        if len(group) > 1:
            at = min(parent.index(c) for c in group)
            for c in sorted(group, key=order.get):
                parent.remove(c)
            for offset, c in enumerate(sorted(group, key=order.get)):
                parent.insert(at + offset, c)

    for ul in root.xpath('.//ul[@data-role="listview"]'):
        if ul in card_set or any(a.tag == 'ul' for a in ul.iterancestors()):
            continue
        items = ul.xpath('./li')
        if len(items) > 1 and len(ul.xpath('./li/a')) != len(items):
            cards.append(ul)
            card_set.add(ul)
    for card in cards:
        card.set('class', (card.get('class', '') + ' lpc-tile').strip())
        card.tag = 'section'
        card.attrib.pop('data-role', None)
        card.attrib.pop('data-inset', None)
        card.attrib.pop('style', None)
    for parent in {c.getparent() for c in cards}:
        group = [c for c in parent if c in card_set]
        if not group:
            continue
        grid = etree.Element('div', {'class': 'lpc-grid'})
        parent.insert(parent.index(group[0]), grid)
        for card in group:
            grid.append(card)
        # Remove old spacer line breaks between what are now grid cells.
        for br in list(parent.xpath('./br')):
            br.drop_tree()

    # Unique IDs and native styled controls avoid duplicate checkbox-N IDs and
    # conflicting jQuery Mobile wrappers on the original matrix/week controls.
    used_labels = set()
    for index, control in enumerate(root.xpath('.//input[@type="checkbox" or @type="radio"]')):
        old = control.get('id')
        label = None
        sibling = control.getnext()
        if sibling is not None and sibling.tag == 'label' and sibling.get('for') == old:
            label = sibling
        if label is None and old:
            scope = next((a for a in control.iterancestors() if a.tag in ('fieldset', 'td', 'li')), control.getparent())
            label = next((n for n in scope.xpath('.//label') if n.get('for') == old and n not in used_labels), None)
        caption = '' if label is None else ' '.join(label.itertext()).strip()
        if label is not None:
            used_labels.add(label)
            label.drop_tree()
        control.set('id', f'lpc-{number}-choice-{index}')
        control.set('data-role', 'none')
        for key in ('style', 'class'):
            control.attrib.pop(key, None)
        wrapper = etree.Element('label', {'class': 'lpc-choice', 'for': control.get('id')})
        control.addprevious(wrapper)
        wrapper.append(control)
        control.tail = None
        if caption:
            text = etree.SubElement(wrapper, 'span')
            text.text = caption
        else:
            control.set('aria-label', control.get('title') or ('Włącz opcję' if control.get('type') == 'checkbox' else 'Wybierz opcję'))

    # Wide data matrices scroll locally rather than stretching the entire page.
    for table in list(root.xpath('.//table[not(ancestor::table)]')):
        if table.get('id') == 'tabela':
            table.set('class', (table.get('class', '') + ' lpc-data-table').strip())
        wrapper = etree.Element('div', {'class': 'lpc-table-wrap'})
        table.addprevious(wrapper)
        wrapper.append(table)
    for node in root.xpath('.//*[@width or @height]'):
        if node.tag not in ('img', 'svg', 'canvas'):
            node.attrib.pop('width', None)
            node.attrib.pop('height', None)
    return ''.join(etree.tostring(n, encoding='unicode', method='html') for n in root)
