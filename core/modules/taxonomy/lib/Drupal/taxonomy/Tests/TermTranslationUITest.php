<?php

/**
 * @file
 * Definition of Drupal\taxonomy\Tests\TermTranslationUITest.
 */

namespace Drupal\taxonomy\Tests;

use Drupal\translation_entity\Tests\EntityTranslationUITest;

/**
 * Tests the Term Translation UI.
 */
class TermTranslationUITest extends EntityTranslationUITest {

  /**
   * The name of the test taxonomy term.
   */
  protected $name;

  /**
   * The vocabulary used for creating terms.
   *
   * @var \Drupal\taxonomy\Plugin\Core\Entity\Vocabulary
   */
  protected $vocabulary;

  /**
   * Modules to enable.
   *
   * @var array
   */
  public static $modules = array('language', 'translation_entity', 'taxonomy');

  public static function getInfo() {
    return array(
      'name' => 'Taxonomy term translation UI',
      'description' => 'Tests the basic term translation UI.',
      'group' => 'Taxonomy',
    );
  }

  function setUp() {
    $this->entityType = 'taxonomy_term';
    $this->bundle = 'tags';
    $this->name = $this->randomName();
    parent::setUp();
  }

  /**
   * Overrides \Drupal\translation_entity\Tests\EntityTranslationUITest::setupBundle().
   */
  protected function setupBundle() {
    parent::setupBundle();

    // Create a vocabulary.
    $this->vocabulary = entity_create('taxonomy_vocabulary', array(
      'name' => $this->bundle,
      'description' => $this->randomName(),
      'vid' => $this->bundle,
      'langcode' => LANGUAGE_NOT_SPECIFIED,
      'help' => '',
      'weight' => mt_rand(0, 10),
    ));
    $this->vocabulary->save();
  }

  /**
   * Overrides \Drupal\translation_entity\Tests\EntityTranslationUITest::getTranslatorPermission().
   */
  function getTranslatorPermissions() {
    return array_merge(parent::getTranslatorPermissions(), array('administer taxonomy'));
  }

  /**
   * Overrides \Drupal\translation_entity\Tests\EntityTranslationUITest::getNewEntityValues().
   */
  protected function getNewEntityValues($langcode) {
    // Term name is not translatable hence we use a fixed value.
    return array('name' => $this->name) + parent::getNewEntityValues($langcode);
  }

  /**
   * Overrides \Drupal\translation_entity\Tests\EntityTranslationUITest::testTranslationUI().
   */
  public function testTranslationUI() {
    parent::testTranslationUI();

    // Make sure that no row was inserted for taxonomy vocabularies, which do
    // not have translations enabled.
    $rows = db_query('SELECT * FROM {translation_entity}')->fetchAll();
    $this->assertEqual(2, count($rows));
    $this->assertEqual('taxonomy_term', $rows[0]->entity_type);
    $this->assertEqual('taxonomy_term', $rows[1]->entity_type);
  }

  /**
   * Tests translate link on vocabulary term list.
   */
  function testTranslateLinkVocabularyAdminPage() {
    $this->admin_user = $this->drupalCreateUser(array_merge(parent::getTranslatorPermissions(), array('access administration pages', 'administer taxonomy')));
    $this->drupalLogin($this->admin_user);

    $translatable_tid = $this->createEntity(array(), $this->langcodes[0], $this->vocabulary->id());

    // Create an untranslatable vocabulary.
    $untranslatable_vocabulary = entity_create('taxonomy_vocabulary', array(
      'name' => 'untranslatable_voc',
      'description' => $this->randomName(),
      'vid' => 'untranslatable_voc',
      'langcode' => LANGUAGE_NOT_SPECIFIED,
      'help' => '',
      'weight' => mt_rand(0, 10),
    ));
    $untranslatable_vocabulary->save();

    $untranslatable_tid = $this->createEntity(array(), $this->langcodes[0], $untranslatable_vocabulary->id());

    // Verify translation links.
    $this->drupalGet('admin/structure/taxonomy/' .  $this->vocabulary->id());
    $this->assertResponse(200);
    $this->assertLinkByHref('term/' . $translatable_tid . '/translations');
    $this->assertLinkByHref('term/' . $translatable_tid . '/edit');

    $this->drupalGet('admin/structure/taxonomy/' . $untranslatable_vocabulary->id());
    $this->assertResponse(200);
    $this->assertLinkByHref('term/' . $untranslatable_tid . '/edit');
    $this->assertNoLinkByHref('term/' . $untranslatable_tid . '/translations');
  }

}
